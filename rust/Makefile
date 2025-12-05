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

include $(APPDIR)/Make.defs

modules_dir=$(APPDIR)/frameworks/runtimes/feature/rust/modules

ifeq ($(CONFIG_FEATURE_RUST_MODULES),y)

ifeq ($(CONFIG_RUST_FEATURE_SYSTEM_EXCHANGE),y)
	RUST_MODULES += system_exchange
endif

ifeq ($(CONFIG_RUST_FEATURE_SYSTEM_NETWORK),y)
	RUST_MODULES += system_network
endif

context::
	$(foreach module,$(RUST_MODULES),$(call nuttx_add_rust,$(module),$(modules_dir)/$(module));)
endif

include $(APPDIR)/Application.mk
