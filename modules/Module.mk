#
# Copyright (C) 2025 Xiaomi Corporation
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

FEATURE_REGISTRY = $(APPDIR)/frameworks/runtimes/feature/modules/registry
FEATURE_LIST_PATH := $(addprefix $(FEATURE_REGISTRY)/,$(addsuffix .pdat,$(FEATURELIST)))
FEATURE_LIST_PATH += $(addprefix $(FEATURE_REGISTRY)/,$(addsuffix .pdat,$(CFEATURELIST)))
AJS_FEATURES_REGISTRY_LIST = $(APPDIR)/frameworks/runtimes/feature/modules/features_registry_list.h
AJS_CFEATURES_REGISTRY_LIST = $(APPDIR)/frameworks/runtimes/feature/modules/cfeatures_registry_list.h
AJS_FEATURES_REGISTRY_TABLE = $(APPDIR)/frameworks/runtimes/feature/modules/features_registry_table.h

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
		$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp $(jidl_path) --out-dir $(out_path) --header $(file_name).h --source $(file_name).cpp; \
	)

	$(foreach i,$(shell seq 1 $(words $(JIDL_C_PATH))), \
		$(eval jidl_path=$(word $(i),$(JIDL_C_PATH))) \
		$(eval out_path=$(word $(i),$(OUT_PATH))) \
		$(eval file_name=$(strip $(basename $(notdir $(word $(i),$(JIDL_C_PATH))) .jidl))) \
		$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp $(jidl_path) --out-dir $(out_path) --header $(file_name).h --source $(file_name).c; \
	)

ifeq ($(wildcard $(AJS_FEATURES_REGISTRY_LIST)),)
	# @echo "generate features_registry_list.h with FEATURELIST: $(FEATURELIST)"
	@touch $(AJS_FEATURES_REGISTRY_LIST)
	$(if $(FEATURELIST),, @echo "FEATURELIST is empty";)
	$(if $(FEATURELIST), \
		$(foreach module, $(sort ${FEATURELIST}), echo "bool jse_${module}_initFeature(FeatureRegistryHandle handle);" >> $(AJS_FEATURES_REGISTRY_LIST);), \
	)
endif

ifeq ($(wildcard $(AJS_CFEATURES_REGISTRY_LIST)),)
	# @echo "generate cfeatures_registry_list.h with CFEATURELIST: $(CFEATURELIST)"+
	@touch $(AJS_CFEATURES_REGISTRY_LIST)
	$(if $(CFEATURELIST),, @echo "CFEATURELIST is empty";)
	$(if $(CFEATURELIST), \
		$(foreach module, $(sort ${CFEATURELIST}), echo "bool jse_${module}_initFeature(FeatureRegistryHandle handle);" >> $(AJS_CFEATURES_REGISTRY_LIST);), \
	)
endif

ifeq ($(wildcard $(AJS_FEATURES_REGISTRY_TABLE)),)
	# @echo "generate features_registry_table.h with FEATURELIST: $(FEATURELIST)"
	@touch $(AJS_FEATURES_REGISTRY_TABLE)
	$(if $(FEATURELIST),, @echo "FEATURELIST is empty";)
	$(if $(FEATURELIST), \
		$(foreach module, $(sort ${FEATURELIST}), echo "jse_${module}_initFeature," >> $(AJS_FEATURES_REGISTRY_TABLE);), \
	)

	$(if $(CFEATURELIST),, @echo "CFEATURELIST is empty";)
	$(if $(CFEATURELIST), \
		$(foreach module, $(sort ${CFEATURELIST}), echo "jse_${module}_initFeature," >> $(AJS_FEATURES_REGISTRY_TABLE);), \
	)
endif

distclean::
	rm -rf $(AJS_FEATURES_REGISTRY_LIST)
	rm -rf $(AJS_CFEATURES_REGISTRY_LIST)
	rm -rf $(AJS_FEATURES_REGISTRY_TABLE)
	$(call DELFILE, $(PDATLIST))

endif
