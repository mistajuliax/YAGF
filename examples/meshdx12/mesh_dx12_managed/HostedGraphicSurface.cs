﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Interop;
using System.Windows.Media;

namespace mesh_dx12_managed
{
    class HostedGraphicSurface : HwndHost
    {
        internal const int
            WS_CHILD = 0x40000000,
             WS_VISIBLE = 0x10000000,
            LBS_NOTIFY = 0x00000001,
            HOST_ID = 0x00000002,
            LISTBOX_ID = 0x00000001,
            WS_VSCROLL = 0x00200000,
            WS_BORDER = 0x00800000;

        [DllImport("user32.dll", EntryPoint = "CreateWindowEx", CharSet = CharSet.Unicode)]
        internal static extern IntPtr CreateWindowEx(int dwExStyle,
                                              string lpszClassName,
                                              string lpszWindowName,
                                              int style,
                                              int x, int y,
                                              int width, int height,
                                              IntPtr hwndParent,
                                              IntPtr hMenu,
                                              IntPtr hInst,
                                              [MarshalAs(UnmanagedType.AsAny)] object pvParam);

        [DllImport("mesh.dx12.dll")]
        internal static extern IntPtr create_vulkan_mesh(IntPtr Hwnd, IntPtr hInstance);
        [DllImport("mesh.dx12.dll")]
        internal static extern void draw_vulkan_mesh(IntPtr mesh_ptr);
        [DllImport("mesh.dx12.dll")]
        internal static extern void destroy_vulkan_mesh(IntPtr mesh_ptr);
        [DllImport("mesh.dx12.dll")]
        internal static extern void set_horizontal_angle(IntPtr mesh_ptr, float angle);
        [DllImport("mesh.dx12.dll")]
        internal static extern IntPtr get_xue(IntPtr mesh_ptr);
        [DllImport("mesh.dx12.dll")]
        internal static extern void set_rotation_on_node(IntPtr node_ptr, float x, float y, float z);
        static IntPtr mesh;
        static IntPtr xue;

        float timer = 0;

        public void set_horizontal_angle(float angle)
        {
            set_horizontal_angle(mesh, angle);
        }

        private void draw(object o, EventArgs e)
        {
            set_rotation_on_node(xue, 0, timer / 360, 0);
            timer += 16;
            draw_vulkan_mesh(mesh);
        }

        protected override HandleRef BuildWindowCore(HandleRef hwndParent)
        {
            // create the window and use the result as the handle
            var hWnd = CreateWindowEx(0,
                "static",    // name of the window class
                "",   // title of the window
                WS_CHILD,    // window style
                0,    // x-position of the window
                0,    // y-position of the window
                1024,    // width of the window
                1024,    // height of the window
                hwndParent.Handle,
                IntPtr.Zero,    // we aren't using menus, NULL
                IntPtr.Zero,    // application handle
                IntPtr.Zero);    // used with multiple windows, NULL

            mesh = create_vulkan_mesh(hWnd, Marshal.GetHINSTANCE(typeof(App).Module));
            xue = get_xue(mesh);
            CompositionTarget.Rendering += draw;
            return new HandleRef(this, hWnd);
        }

        protected override void DestroyWindowCore(HandleRef hwnd)
        {
            destroy_vulkan_mesh(mesh);
        }
    }
}
