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
  void Interface_onRegister(FeatureRuntimeContext ctx);
  void Interface_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Interface_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Interface_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void Interface_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void Interface_onUnregister(FeatureRuntimeContext ctx);

  // Struct defines

  // Function wrappers to be implemented
  void Interface_wrap_flyFar(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid, FtInt distance);
  FeatureInstanceHandle Interface_wrap_createCat(FeatureInstanceHandle feature, AppendData data);
  void Interface_wrap_setAnimal(FeatureInstanceHandle feature, AppendData data, FeatureInstanceHandle animal);
  void Interface_wrap_print(FeatureInstanceHandle feature, AppendData data, FtVariParams vari_params);

  // Property getters and setters to be implemented

  // interface vtable functions to be implemented
  // vtable functions for interface constructor function 'createDog'
  FtString Interface_Animal_interface_dog_get_name(FeatureInstanceHandle feature, AppendData data);
  void Interface_Animal_interface_dog_set_name(FeatureInstanceHandle feature, AppendData data, FtString name);
  FtInt Interface_Animal_interface_dog_get_legCount(FeatureInstanceHandle feature, AppendData data);
  FtInt Interface_Animal_interface_dog_eatFood(FeatureInstanceHandle feature, AppendData data, FtArray& foods);
  FtString Interface_Animal_interface_dog_run(FeatureInstanceHandle feature, AppendData data, FtInt distance, FtString destination);
  FtArray* Interface_Animal_interface_dog_fly(FeatureInstanceHandle feature, AppendData data);
  void Interface_Animal_interface_dog_walk(FeatureInstanceHandle feature, AppendData data, FtPromiseId pid);


  // Array malloc functions
  FtArray* Interface_malloc_string_array();

#endif // JSON_AST_GEN_MODULE_INTERFACE_H_
