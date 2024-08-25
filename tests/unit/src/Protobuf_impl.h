#pragma once
#include "Protobuf.h"
#include "feature_types.h"
#include "test.pb-c.h"

namespace Feature_Protobuf {
// 使用CRTP进行静态绑定
class Protobuf : public ProtobufBase {
public:
    Protobuf(FeatureInstanceHandle hInstance);
    ~Protobuf() = default;
    static inline Protobuf* Create(FeatureInstanceHandle hInst) { return new Protobuf(hInst); }
    void proto(FtInt a, const Pb_computer_p& b) override;
    void invoke_proto(FtCallbackId cb) override;
};

}