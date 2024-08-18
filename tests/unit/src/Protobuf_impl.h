#pragma once
#include "Protobuf.h"
#include "feature_types.h"
#include "test.pb-c.h"

namespace Feature_Protobuf {
// 使用CRTP进行静态绑定
class Protobuf : public ProtobufBase {
private:
    FeatureInstanceHandle hInst_ = nullptr;
public:
    FeatureInstanceHandle getHandle() const override { return hInst_; }
    Protobuf(FeatureInstanceHandle hInstance);
    ~Protobuf() = default;
    static inline Protobuf* newInstance(FeatureInstanceHandle hInst) { return new Protobuf(hInst); }
    void proto(AppendData append_data, FtInt a, Computer* b) override;
    void proto_cb(AppendData append_data, FtCallbackId cb) override;
};

}