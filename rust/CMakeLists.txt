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
  nuttx_add_library(rust_features_impl)

  if(CONFIG_RUST_FEATURE_SYSTEM_EXCHANGE)
    list(APPEND MODULES ${CMAKE_CURRENT_LIST_DIR}/modules/system_exchange)
  endif()

  if(CONFIG_RUST_FEATURE_SYSTEM_NETWORK)
    list(APPEND MODULES ${CMAKE_CURRENT_LIST_DIR}/modules/system_network)
  endif()

  if(CONFIG_FEATURE_RUST_MODULES_UNSTABLE)
    list(APPEND MODULES ${CMAKE_CURRENT_LIST_DIR}/modules/system_schedule
         ${CMAKE_CURRENT_LIST_DIR}/modules/system_media_audio)
  endif()

  foreach(MODULE ${MODULES})
    get_filename_component(MODULE_NAME ${MODULE} NAME)
    set(JIDL_PATH ${MODULE}/jidl/${MODULE_NAME}.jidl)
    nuttx_add_jidl_rust(
      TARGET
      rust_features_impl
      JIDLS
      ${JIDL_PATH}
      FEATURE_CRATE
      ${MODULE}
      FEATURE_NAMES
      ${MODULE_NAME})
  endforeach()

endif(CONFIG_FEATURE_RUST_MODULES)
