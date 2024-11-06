#pragma once
#include "Struct.h"

namespace Feature_Struct {
class Struct : public StructBase {

public:
    Struct(FeatureInstanceHandle hInstance, int a, int b);
    ~Struct();
    static inline Struct* Create(FeatureInstanceHandle hInst) { return new Struct(hInst, 1, 2); }
    void foo(FtInt a, const ft_utils::RefPtr<Chapter>& b) override;
    class ft_utils::RefPtr<Chapter> bar(FtInt a) override;
    void bar2(const ft_utils::RefPtr<Book>& a) override;
    ft_utils::RefPtr<FtArray> getBooks(FtInt count) override;

private:
    // 用户可以按需定义类，保存逻辑必要的数据
    int a_ = 0;
    int b_ = 0;
};

}