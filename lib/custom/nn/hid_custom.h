/**
 * @file hid.h
 * @brief Functions that help process gamepad inputs.
 */

 #pragma once

 #include <nn/types.h>
 #include <nn/util.h>
 #include <nn/util/MathTypes.h>
 #include <nn/util/util_BitFlagSet.h>
 #include <nn/util/util_BitPack.h>
 #include <nn/hid.h>
 
 namespace nn {
 namespace hid {

 template<size_t T>
 struct TouchScreenState {
     u64 samplingNumber = 0;
     s32 count = T;
     char reserved[4] = {};
     TouchState touches[T] = {};
 };

 enum KeyboardLayout {

 };

 template<size_t T>
 void GetTouchScreenState(nn::hid::TouchScreenState<T> *);

 void GetKeyCode(unsigned short *, int, nn::util::BitFlagSet<32, nn::hid::KeyboardModifier>,
                 nn::util::BitPack<unsigned int, nn::hid::KeyboardLayout>);
 
 }  // namespace hid
 }  // namespace nn
 