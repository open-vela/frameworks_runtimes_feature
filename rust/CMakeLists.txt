# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_FEATURE_RUST_MODULES)
include(nuttx_add_rust)

nuttx_add_library(rust_features_impl)

file(GLOB MODULES "modules/*")
# 遍历所有模块目录
foreach(MODULE ${MODULES})
    get_filename_component(MODULE_NAME ${MODULE} NAME)
    set(JIDL_PATH ${MODULE}/jidl/${MODULE_NAME}.jidl)
    # 为每个模块配置 Cargo 构建
    nuttx_add_jidl_rust(
        TARGET
        rust_features_impl
        JIDLS
        ${JIDL_PATH}
        FEATURE_CRATE
        ${MODULE}
        FEATURE_NAMES
        ${MODULE_NAME}
    )
endforeach()

endif(CONFIG_FEATURE_RUST_MODULES)