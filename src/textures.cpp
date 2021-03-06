// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <Scene/textures.h>
#include <gli/gli.hpp>

std::tuple<std::unique_ptr<image_t>, std::unique_ptr<buffer_t>>
load_texture(device_t &dev, std::string &&texture_name,
             command_list_t &upload_command_list) {
  const auto &DDSPic = gli::load(texture_name);

  const auto &width = static_cast<uint32_t>(DDSPic.extent().x);
  const auto &height = static_cast<uint32_t>(DDSPic.extent().y);
  const auto &mipmap_count = static_cast<uint16_t>(DDSPic.levels());

  const auto &is_cubemap = gli::is_target_cube(DDSPic.target());
  uint16_t layer_count = is_cubemap ? 6 : 1;

  auto &&upload_buffer = dev.create_buffer(
      width * height * 3 * 6, irr::video::E_MEMORY_POOL::EMP_CPU_WRITEABLE,
      usage_buffer_transfer_src);

  void *pointer = upload_buffer->map_buffer();

  size_t offset_in_texram = 0;

  uint32_t block_height = 4;
  uint32_t block_width = 4;
  uint32_t block_size = 8;
  std::vector<MipLevelData> Mips;
  for (unsigned face = 0; face < layer_count; face++) {
    for (unsigned i = 0; i < mipmap_count; i++) {
      // Offset needs to be aligned to 512 bytes
      offset_in_texram = (offset_in_texram + 511) & ~511;
      // Row pitch is always a multiple of 256
      uint32_t height_in_blocks =
          static_cast<uint32_t>(DDSPic.extent(i).y + block_height - 1) /
          block_height;
      uint32_t width_in_blocks =
          static_cast<uint32_t>(DDSPic.extent(i).x + block_width - 1) /
          block_width;
      uint32_t height_in_texram = height_in_blocks * block_height;
      uint32_t width_in_texram = width_in_blocks * block_width;
      uint32_t rowPitch = width_in_blocks * block_size;
      rowPitch = (rowPitch + 255) & ~255;
      const auto &mml = MipLevelData{offset_in_texram, width_in_texram,
                                     height_in_texram, rowPitch};
      Mips.push_back(mml);
      for (unsigned row = 0; row < height_in_blocks; row++) {
        memcpy(((char *)pointer) + offset_in_texram,
               (char *)DDSPic.data(0, face, i) +
                   row * width_in_blocks * block_size,
               width_in_blocks * block_size);
        offset_in_texram += rowPitch;
      }
    }
  }
  upload_buffer->unmap_buffer();

  std::unique_ptr<image_t> texture = dev.create_image(
      irr::video::ECF_BC1_UNORM_SRGB, width, height, mipmap_count, layer_count,
      usage_sampled | usage_transfer_dst | (is_cubemap ? usage_cube : 0),
      nullptr);

  uint32_t miplevel = 0;
  for (const MipLevelData &mipmapData : Mips) {
    upload_command_list.set_pipeline_barrier(
        *texture, RESOURCE_USAGE::undefined, RESOURCE_USAGE::COPY_DEST,
        miplevel, irr::video::E_ASPECT::EA_COLOR);
    upload_command_list.copy_buffer_to_image_subresource(
        *texture, miplevel, *upload_buffer, mipmapData.Offset, mipmapData.Width,
        mipmapData.Height, mipmapData.RowPitch, irr::video::ECF_BC1_UNORM_SRGB);
    upload_command_list.set_pipeline_barrier(
        *texture, RESOURCE_USAGE::COPY_DEST, RESOURCE_USAGE::READ_GENERIC,
        miplevel, irr::video::E_ASPECT::EA_COLOR);
    miplevel++;
  }
  return std::make_tuple(std::move(texture), std::move(upload_buffer));
}