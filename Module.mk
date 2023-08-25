#
# Copyright (C) 2023 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
ifeq ($(CONFIG_FEATURE_FRAMEWORK),y)

.PHONY: all clean distclean depend register context

FEATURE_REGISTRY = $(APPDIR)/frameworks/base/feature/registry
FEATURE_LIST_PATH := $(addprefix $(FEATURE_REGISTRY)/,$(addsuffix .pdat,$(FEATURELIST)))

#打印FEATURE_LIST_PATH
#$(info FEATURE_LIST_PATH is ${FEATURE_LIST_PATH})

$(FEATURE_LIST_PATH): $(DEPCONFIG) Makefile
	touch $@

register:: $(FEATURE_LIST_PATH)

context::
	@rm -rf $(APPDIR)/frameworks/base/feature/include/ajs_features_init.h
	@rm -rf $(APPDIR)/frameworks/base/feature/include/ajs_features_list.h
	@echo "#include \"feature_manager.h\"" > $(APPDIR)/frameworks/base/feature/include/ajs_features_init.h
	@echo "" >> $(APPDIR)/frameworks/base/feature/include/ajs_features_init.h
	@echo "using namespace ferry;" >> $(APPDIR)/frameworks/base/feature/include/ajs_features_init.h
	@echo "#undef QAPPFEATURE_INIT" >> $(APPDIR)/frameworks/base/feature/include/ajs_features_init.h
	@echo "#define QAPPFEATURE_INIT(module) bool jse_##module##_initFeature(ferry::FeatureManager *mgr, std::vector<std::string>&features)" >> $(APPDIR)/frameworks/base/feature/include/ajs_features_init.h
	@echo "" >> $(APPDIR)/frameworks/base/feature/include/ajs_features_init.h

ifeq ($(FEATURELIST),)
	@echo "FEATURELIST is empty"
	@echo "" > $(APPDIR)/frameworks/base/feature/include/ajs_features_list.h
else
	@echo "FEATURELIST is not empty"
	@$(foreach module,  $(sort ${FEATURELIST}), echo "jse_${module}_initFeature(this, features);" >> $(APPDIR)/frameworks/base/feature/include/ajs_features_list.h;)
	@$(foreach module,  $(sort ${FEATURELIST}), echo "bool jse_${module}_initFeature(ferry::FeatureManager *mgr, std::vector<std::string>&features);" >> $(APPDIR)/frameworks/base/feature/include/ajs_features_init.h;)
endif

endif