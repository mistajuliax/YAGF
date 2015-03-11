// Copyright (C) 2015 Vincent Lejeune

#ifndef SINGLETON_H
#define SINGLETON_H

template <typename Type>
class Singleton
{
private:
  static Type *Instance;
public:
  static Type *getInstance()
  {
    if (Instance == nullptr)
      Instance = new Type();
    return Instance;
  }

  static void kill()
  {
    if (Instance != nullptr)
      delete Instance;
  }
};

template <typename Type> Type *Singleton<Type>::Instance = nullptr;

#endif