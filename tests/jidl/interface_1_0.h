// Copyright 2023 Xiaomi, Inc. All rights reserved.




#ifndef JSON_AST_GEN_MODULE_INTERFACE_H_
#define JSON_AST_GEN_MODULE_INTERFACE_H_

#include "feature_exports.h"
#include "feature_log.h"

#include <ffi.h>
#include <assert.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  // FeatureCallbacks to be implemented
  void Interface_onRegister(const char* feature_name);
  void Interface_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Interface_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Interface_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Interface_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Interface_onUnregister(const char* feature_name);

  // Struct defines

  // Function wrappers to be implemented
  FeatureInterfaceHandle Interface_wrap_createDog(FeatureInstanceHandle feature, AppendData data, FtInt type);
  FeatureInterfaceHandle Interface_wrap_createPigeon(FeatureInstanceHandle feature, AppendData data);
  FeatureInterfaceHandle Interface_wrap_createCock(FeatureInstanceHandle feature, AppendData data);
  FeatureInterfaceHandle Interface_wrap_createCat(FeatureInstanceHandle feature, AppendData data);
  void Interface_wrap_setAnimal(FeatureInstanceHandle feature, AppendData data, FeatureInterfaceHandle animal);
  void Interface_wrap_flyFar(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid, FtInt distance);
  void Interface_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariParams vari_params);

  // Interface constructors
  FeatureInterfaceHandle Interface_createDog_instance(FeatureInstanceHandle feature);
  FeatureInterfaceHandle Interface_createPigeon_instance(FeatureInstanceHandle feature);
  FeatureInterfaceHandle Interface_createCock_instance(FeatureInstanceHandle feature);

  // interface vtable functions to be implemented
  // vtable functions for interface constructor function 'createDog'
  void Interface_Animal_interface_dog_finalize(FeatureInterfaceHandle handle);
  FtString Interface_Animal_interface_dog_get_name(FeatureInterfaceHandle handle, AppendData data);
  void Interface_Animal_interface_dog_set_name(FeatureInterfaceHandle handle, AppendData data, FtString name);
  FtInt Interface_Animal_interface_dog_get_legCount(FeatureInterfaceHandle handle, AppendData data);
  FtInt Interface_Animal_interface_dog_eatFood(FeatureInterfaceHandle handle, AppendData data, FtArray& foods);
  FtString Interface_Animal_interface_dog_run(FeatureInterfaceHandle handle, AppendData data, FtInt distance, FtString destination);

  // vtable functions for interface constructor function 'createPigeon'
  void Interface_Bird_interface_pigeon_finalize(FeatureInterfaceHandle handle);
  FtArray* Interface_Bird_interface_pigeon_fly(FeatureInterfaceHandle handle, AppendData data);
  FtString Interface_Bird_interface_pigeon_get_breed(FeatureInterfaceHandle handle, AppendData data);
  void Interface_Bird_interface_pigeon_set_breed(FeatureInterfaceHandle handle, AppendData data, FtString breed);

  // vtable functions for interface constructor function 'createCock'
  void Interface_Chicken_interface_cock_finalize(FeatureInterfaceHandle handle);
  FtString Interface_Chicken_interface_cock_get_name(FeatureInterfaceHandle handle, AppendData data);
  void Interface_Chicken_interface_cock_set_name(FeatureInterfaceHandle handle, AppendData data, FtString name);
  FtInt Interface_Chicken_interface_cock_get_legCount(FeatureInterfaceHandle handle, AppendData data);
  FtInt Interface_Chicken_interface_cock_eatFood(FeatureInterfaceHandle handle, AppendData data, FtArray& foods);
  FtString Interface_Chicken_interface_cock_run(FeatureInterfaceHandle handle, AppendData data, FtInt distance, FtString destination);
  FtArray* Interface_Chicken_interface_cock_fly(FeatureInterfaceHandle handle, AppendData data);
  FtString Interface_Chicken_interface_cock_get_breed(FeatureInterfaceHandle handle, AppendData data);
  void Interface_Chicken_interface_cock_set_breed(FeatureInterfaceHandle handle, AppendData data, FtString breed);
  FtInt Interface_Chicken_interface_cock_get_weight(FeatureInterfaceHandle handle, AppendData data);
  void Interface_Chicken_interface_cock_set_weight(FeatureInterfaceHandle handle, AppendData data, FtInt weight);
  void Interface_Chicken_interface_cock_walk(FeatureInterfaceHandle handle, AppendData data, FtPromiseId pid);


  // Property getters and setters to be implemented

  // Array malloc functions
  FtArray* Interface_malloc_string_array();

#endif // JSON_AST_GEN_MODULE_INTERFACE_H_
