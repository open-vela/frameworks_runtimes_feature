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
FEATURE_LIST_PATH += $(addprefix $(FEATURE_REGISTRY)/,$(addsuffix .pdat,$(CFEATURELIST)))

#打印FEATURE_LIST_PATH
#$(info FEATURE_LIST_PATH is ${FEATURE_LIST_PATH})

$(FEATURE_LIST_PATH): $(DEPCONFIG) Makefile
	touch $@

register:: $(FEATURE_LIST_PATH)

# context::
depend::
	@echo "-------------------generate files----------------------"
	$(foreach i,$(shell seq 1 $(words $(JIDL_PATH))), \
		$(eval jidl_path=$(word $(i),$(JIDL_PATH))) \
		$(eval out_path=$(word $(i),$(OUT_PATH))) \
		$(eval file_name=$(strip $(basename $(notdir $(word $(i),$(JIDL_PATH))) .jidl))) \
		echo "python3 $(APPDIR)/frameworks/base/feature/tools/jidl/jsongensource.py $(jidl_path) -out-dir $(out_path) -header $(file_name).h -source $(file_name).cpp"; \
		python3 $(APPDIR)/frameworks/base/feature/tools/jidl/jsongensource.py $(jidl_path) -out-dir $(out_path) -header $(file_name).h -source $(file_name).cpp; \
	)

ifeq ($(wildcard $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h),)
	@echo "ajs_features_init.h is empty, need create it"
	@echo "#include \"feature_exports.h\"" > $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h
	@echo "" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h
	@echo "#undef QAPPFEATURE_INIT" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h
	@echo "#define QAPPFEATURE_INIT(module) bool jse_##module##_initFeature(FeatureRegistryHandle handle)" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h
	@echo "" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h
endif

ifeq ($(wildcard $(APPDIR)/frameworks/base/feature/src/ajs_features_list.h),)
	@echo "ajs_features_list.h is empty, need create it"
	@echo "" > $(APPDIR)/frameworks/base/feature/src/ajs_features_list.h
endif

ifeq ($(FEATURELIST),)
	@echo "FEATURELIST is empty"
	@echo "" > $(APPDIR)/frameworks/base/feature/src/ajs_features_list.h
else
	@echo "FEATURELIST is not empty"
	@$(foreach module,  $(sort ${FEATURELIST}), echo "jse_${module}_initFeature(handle);" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_list.h;)
	@$(foreach module,  $(sort ${FEATURELIST}), echo "bool jse_${module}_initFeature(FeatureRegistryHandle handle);" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h;)
endif

ifeq ($(CFEATURELIST),)
	@echo "CFEATURELIST is empty"
else
	@echo "CFEATURELIST is not empty"
	@$(foreach module,  $(sort ${CFEATURELIST}), echo "jse_${module}_initFeature(handle);" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_list.h;)
	@echo "#ifdef __cplusplus" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h
	@echo "extern \"C\" {" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h
	@echo "#endif" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h
	@$(foreach module,  $(sort ${CFEATURELIST}), echo "    bool jse_${module}_initFeature(FeatureRegistryHandle handle);" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h;)
	@echo "#ifdef __cplusplus" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h
	@echo "}" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h
	@echo "#endif" >> $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h
endif

distclean::
	rm -rf $(APPDIR)/frameworks/base/feature/src/ajs_features_list.h
	rm -rf $(APPDIR)/frameworks/base/feature/src/ajs_features_init.h
	$(call DELFILE, $(PDATLIST))

endif
