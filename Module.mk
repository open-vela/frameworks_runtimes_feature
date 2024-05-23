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
AJS_FEATURES_REGISTRY = $(APPDIR)/frameworks/base/feature/src/ajs_features_registry.cpp
AJS_FEATURES_LIST = $(APPDIR)/frameworks/base/feature/src/ajs_features_list.h

#打印FEATURE_LIST_PATH
#$(info FEATURE_LIST_PATH is ${FEATURE_LIST_PATH})

$(FEATURE_LIST_PATH): $(DEPCONFIG) Makefile
	touch $@

register:: $(FEATURE_LIST_PATH)

# context::
depend::
	# @echo "-------------------generate files----------------------"
	$(foreach i,$(shell seq 1 $(words $(JIDL_PATH))), \
		$(eval jidl_path=$(word $(i),$(JIDL_PATH))) \
		$(eval out_path=$(word $(i),$(OUT_PATH))) \
		$(eval file_name=$(strip $(basename $(notdir $(word $(i),$(JIDL_PATH))) .jidl))) \
		python3 $(APPDIR)/frameworks/base/feature/tools/jidl/jsongensource.py $(jidl_path) -out-dir $(out_path) -header $(file_name).h -source $(file_name).cpp; \
	)

ifeq ($(wildcard $(AJS_FEATURES_REGISTRY)),)
	# @echo "generate ajs_features_registry.cpp with FEATURELIST: $(FEATURELIST)"
	@echo "#include \"ajs_features_registry.h\"" >> $(AJS_FEATURES_REGISTRY)
	@echo "#include \"ajs_features_list.h\"" >> $(AJS_FEATURES_REGISTRY)
	@echo "" >> $(AJS_FEATURES_REGISTRY)
	@echo "bool registerAjsFeatures(FeatureRegistryHandle handle) {" >> $(AJS_FEATURES_REGISTRY)
	$(if $(FEATURELIST),, @echo "FEATURELIST is empty";)
	$(if $(FEATURELIST), \
		$(foreach module, $(sort ${FEATURELIST}), echo "    jse_${module}_initFeature(handle);" >> $(AJS_FEATURES_REGISTRY);), \
	)
	@echo "    return true;" >> $(AJS_FEATURES_REGISTRY)
	@echo "}" >> $(AJS_FEATURES_REGISTRY)
endif

ifeq ($(wildcard $(AJS_FEATURES_LIST)),)
	# @echo "generate ajs_features_list.h with FEATURELIST: $(FEATURELIST)"
	@echo "#ifndef AJS_FEATURES_LIST_H_" >> $(AJS_FEATURES_LIST)
	@echo "#define AJS_FEATURES_LIST_H_" >> $(AJS_FEATURES_LIST)
	@echo "" >> $(AJS_FEATURES_LIST)
	@echo "#include \"feature_exports.h\"" >> $(AJS_FEATURES_LIST)
	@echo "" >> $(AJS_FEATURES_LIST)
	$(if $(FEATURELIST),, @echo "FEATURELIST is empty";)
	$(if $(FEATURELIST), \
		$(foreach module, $(sort ${FEATURELIST}), echo "bool jse_${module}_initFeature(FeatureRegistryHandle handle);" >> $(AJS_FEATURES_LIST);), \
	)
	@echo "" >> $(AJS_FEATURES_LIST)
	@echo "#ifdef __cplusplus" >> $(AJS_FEATURES_LIST)
	@echo "extern \"C\" {" >> $(AJS_FEATURES_LIST)
	@echo "#endif" >> $(AJS_FEATURES_LIST)
	$(if $(CFEATURELIST),, @echo "CFEATURELIST is empty";)
	$(if $(CFEATURELIST), \
		$(foreach module, $(sort ${CFEATURELIST}), echo "bool jse_${module}_initFeature(FeatureRegistryHandle handle);" >> $(AJS_FEATURES_LIST);), \
	)
	@echo "#ifdef __cplusplus" >> $(AJS_FEATURES_LIST)
	@echo "}" >> $(AJS_FEATURES_LIST)
	@echo "#endif" >> $(AJS_FEATURES_LIST)
	@echo "" >> $(AJS_FEATURES_LIST)
	@echo "#endif" >> $(AJS_FEATURES_LIST)
endif

distclean::
	rm -rf $(AJS_FEATURES_LIST)
	rm -rf $(AJS_FEATURES_REGISTRY)
	$(call DELFILE, $(PDATLIST))

endif
