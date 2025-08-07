/*
 * Copyright (C) 2023 Xiaomi Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "imc.h"

#include "ash/logging/logging.h"
#include "ash/memory/weak_ptr.h"
#include "ash/message_loop/message_loop.h"
#include "framework/application.h"
#include "framework/input_method/input_method_manager.h"
#include <functional>
#include <memory>

class ImcOnUI;

class Imc : public ash::SupportsWeakPtr<Imc> {
public:
    Imc(FeatureInstanceHandle feature, std::shared_ptr<ash::TaskRunner> ui_runner);
    ~Imc();

    void UpdateInputText(const std::string& text);
    void SetOnExtractedTextChanged(std::function<void(const std::string&)> cb);
    void Close();

    void UpdateInitialText(const std::string& text);

private:
    FeatureInstanceHandle feature_;
    std::unique_ptr<ImcOnUI> imc_on_ui_;
    std::shared_ptr<ash::TaskRunner> ui_runner_;
    std::function<void(const std::string&)> on_extracted_text_changed_;
};

class ImcOnUI : ferry::InputMethodController {
public:
    ImcOnUI(ash::WeakPtr<Imc> owner, std::shared_ptr<ash::TaskRunner> owner_runner);
    ~ImcOnUI();

    void Init();
    void UpdateInputText(const std::string& text);
    void Close();

    void Bind() override;
    void Unbind() override;
    void UpdateInitialText(const std::string& text) override;

private:
    ash::WeakPtr<Imc> owner_;
    std::shared_ptr<ash::TaskRunner> owner_runner_;
    bool bound_;
};

Imc::Imc(FeatureInstanceHandle feature, std::shared_ptr<ash::TaskRunner> ui_runner)
    : SupportsWeakPtr(this)
    , feature_(feature)
    , imc_on_ui_(std::make_unique<ImcOnUI>(AsWeakPtr(),
          ash::MessageLoop::Current()->GetTaskRunner()))
    , ui_runner_(std::move(ui_runner))
{
    ASH_LOG("IMC", INFO) << "Imc::Imc";
    ui_runner_->PostTask([imc_on_ui = imc_on_ui_.get()]() {
        imc_on_ui->Init();
    });
}

Imc::~Imc()
{
    ASH_LOG("IMC", INFO) << "Imc::~Imc";
    ui_runner_->PostTask([imc_on_ui = std::move(imc_on_ui_)]() {});
}

void Imc::UpdateInputText(const std::string& text)
{
    ASH_LOG("IMC", INFO) << "Imc::UpdateInputText";
    ui_runner_->PostTask([imc_on_ui = imc_on_ui_.get(), text]() {
        imc_on_ui->UpdateInputText(text);
    });
}

void Imc::SetOnExtractedTextChanged(std::function<void(const std::string&)> cb)
{
    ASH_LOG("IMC", INFO) << "Imc::SetOnExtractedTextChanged";
    on_extracted_text_changed_ = std::move(cb);
}

void Imc::Close()
{
    ASH_LOG("IMC", INFO) << "Imc::Close";
    ui_runner_->PostTask([imc_on_ui = imc_on_ui_.get()]() {
        imc_on_ui->Close();
    });
}

void Imc::UpdateInitialText(const std::string& text)
{
    ASH_LOG("IMC", INFO) << "Imc::UpdateInitialText";
    if (on_extracted_text_changed_) {
        on_extracted_text_changed_(text);
    }
}

ImcOnUI::ImcOnUI(ash::WeakPtr<Imc> owner, std::shared_ptr<ash::TaskRunner> owner_runner)
    : owner_(std::move(owner))
    , owner_runner_(std::move(owner_runner))
    , bound_(false)
{
    ASH_LOG("IMC", INFO) << "ImcOnUI::ImcOnUI";
}

ImcOnUI::~ImcOnUI()
{
    ASH_LOG("IMC", INFO) << "ImcOnUI::~ImcOnUI";
    Close();
}

void ImcOnUI::Init()
{
    ASH_LOG("IMC", INFO) << "ImcOnUI::Init";
    ferry::InputMethodManager* imm = ferry::InputMethodManager::GetInstance();
    if (imm && !bound_) {
        imm->BindController(this);
    }
}

void ImcOnUI::UpdateInputText(const std::string& text)
{
    ASH_LOG("IMC", INFO) << "ImcOnUI::UpdateInputText";
    ferry::InputMethodManager* imm = ferry::InputMethodManager::GetInstance();
    if (imm && bound_) {
        imm->UpdateInputText(text);
    }
}

void ImcOnUI::Close()
{
    ASH_LOG("IMC", INFO) << "ImcOnUI::Close";
    ferry::InputMethodManager* imm = ferry::InputMethodManager::GetInstance();
    if (imm && bound_) {
        imm->UnbindController(this);
    }
}

void ImcOnUI::Bind()
{
    ASH_LOG("IMC", INFO) << "ImcOnUI::Bind";
    bound_ = true;
}

void ImcOnUI::Unbind()
{
    ASH_LOG("IMC", INFO) << "ImcOnUI::Unbind";
    bound_ = false;
}

void ImcOnUI::UpdateInitialText(const std::string& text)
{
    ASH_LOG("IMC", INFO) << "ImcOnUI::UpdateInitialText";
    owner_runner_->PostTask([owner = owner_, text]() {
        Imc* imc = owner.Get();
        if (imc) {
            imc->UpdateInitialText(text);
        }
    });
}

void system_imc_onRegister(const char* feature_name)
{
}

void system_imc_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
}

void system_imc_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    // TODO(xuyan): Find a better way to get ui task runner.
    Application* app = static_cast<Application*>(FeatureInstanceGetManagerUserData(handle, "app"));
    std::shared_ptr<ash::TaskRunner> ui_runner = app->getUITaskRunner();
    std::unique_ptr<Imc> imc = std::make_unique<Imc>(handle, std::move(ui_runner));
    FeatureSetObjectData(handle, imc.release());
}

void system_imc_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    std::unique_ptr<Imc> imc(reinterpret_cast<Imc*>(FeatureGetObjectData(handle)));
}

void system_imc_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
}

void system_imc_onUnregister(const char* feature_name)
{
}

void system_imc_wrap_updateInputText(FeatureInstanceHandle handle, AppendData adata, FtString text)
{
    reinterpret_cast<Imc*>(FeatureGetObjectData(handle))->UpdateInputText(text ? text : "");
}

void system_imc_wrap_setOnExtractedTextChanged(FeatureInstanceHandle handle, AppendData adata, FtCallbackId cb)
{
    reinterpret_cast<Imc*>(FeatureGetObjectData(handle))->SetOnExtractedTextChanged([handle, cb](const std::string& text) {
        system_imc_ExtractedTextChangedCB_invoke(handle, cb, text.c_str());
    });
}

void system_imc_wrap_close(FeatureInstanceHandle handle, AppendData adata)
{
    reinterpret_cast<Imc*>(FeatureGetObjectData(handle))->Close();
}
