#pragma once
#include "Struct.h"
#include "feature_types.h"
#include "test.pb-c.h"

namespace Feature_Struct {
class Struct : public StructBase {
private:
    FeatureInstanceHandle hInst_ = nullptr;

public:
    FeatureInstanceHandle getHandle() const override { return hInst_; }
    Struct(FeatureInstanceHandle hInstance, int a, int b);
    ~Struct();
    static inline Struct* newInstance(FeatureInstanceHandle hInst) { return new Struct(hInst, 1, 2); }
    void foo(AppendData append_data, FtInt a, struct Chapter* b) override;
    class Chapter* bar(AppendData append_data, FtInt a) override;
    void bar2(AppendData append_data, struct Book* a) override;
    void proto(FeatureInstanceHandle feature, AppendData append_data, FtInt a, Computer* b);
    void print(AppendData append_data, FtVariParams vari_params) override;
    void proto(AppendData append_data, FtInt a, Computer* b) override;
    void proto_cb(AppendData append_data, FtCallbackId cb) override;

private:
    // 用户可以按需定义类，保存逻辑必要的数据
    int a_ = 0;
    int b_ = 0;
};

}