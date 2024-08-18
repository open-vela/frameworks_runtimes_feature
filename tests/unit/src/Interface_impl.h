#pragma once
#include "Interface.h"
#include "feature_types.h"
#include "utils/feature_utils.h"

namespace Feature_Interface {

class cock : public IChicken {
private:
    FeatureInstanceHandle _hInst = nullptr;

private:
    FtInt _weight = 100;
    FtInt _type = 0;
    ft_utils::FtStringPtr _name;
    ft_utils::FtStringPtr _breed;
    FtInt _legCount = 4;
    FtInt _eatFood = 100;

public:
    inline FeatureInstanceHandle getHandle() const override { return _hInst; }
    cock(FeatureInstanceHandle hInstance);
    // IAnimal
    ft_utils::FtStringPtr name(AppendData append_data) const override;
    void setName(AppendData append_data, const ft_utils::FtStringPtr& val) override;
    FtInt legCount(AppendData append_data) const override;
    FtInt eatFood(AppendData append_data, const ft_utils::RefPtr<FtArray>& food) override;
    ft_utils::FtStringPtr run(AppendData append_data, FtInt distance, const ft_utils::FtStringPtr& destination) override;
    // IBird
    ft_utils::RefPtr<FtArray> fly(AppendData append_data) override;
    ft_utils::FtStringPtr breed(AppendData append_data) const override;
    void setBreed(AppendData append_data, const ft_utils::FtStringPtr& breed) override;
    // IChicken
    FtInt weight(AppendData append_data) const override;
    void setWeight(AppendData append_data, FtInt weight) override;
    void walk(AppendData append_data, FtPromiseId pid) override;
};

class dog : public IAnimal {
private:
    FeatureInstanceHandle _hInst = nullptr;

private:
    FtInt _type = 0;
    ft_utils::FtStringPtr _name;
    FtInt _legCount = 4;
    FtInt _eatFood = 100;

public:
    inline FeatureInstanceHandle getHandle() const override { return _hInst; }
    dog(FeatureInstanceHandle hInstance, FtInt type);
    ~dog();
    ft_utils::FtStringPtr name(AppendData append_data) const override;
    void setName(AppendData append_data, const ft_utils::FtStringPtr& val) override;
    FtInt legCount(AppendData append_data) const override;
    FtInt eatFood(AppendData append_data, const ft_utils::RefPtr<FtArray>& food) override;
    virtual ft_utils::FtStringPtr run(AppendData append_data, FtInt distance, const ft_utils::FtStringPtr& destination) override;
};

class cat : public dog {
public:
    cat(FeatureInstanceHandle hInstance);
    ft_utils::FtStringPtr run(AppendData append_data, FtInt distance, const ft_utils::FtStringPtr& destination) override;

};

class pigeon : public IBird {
private:
    FeatureInstanceHandle _hInst = nullptr;
    ft_utils::FtStringPtr _breed;

public:
    inline FeatureInstanceHandle getHandle() const override { return _hInst; }
    pigeon(FeatureInstanceHandle hInstance)
        : _hInst(hInstance)
    {
    }
    ~pigeon();
    ft_utils::RefPtr<FtArray> fly(AppendData append_data) override;
    ft_utils::FtStringPtr breed(AppendData append_data) const override;
    void setBreed(AppendData append_data, const ft_utils::FtStringPtr& breed) override;
};

class Interface : public InterfaceBase {
private:
    FeatureInstanceHandle _hInst = nullptr;

public:
    Interface(FeatureInstanceHandle hInstance)
        : _hInst(hInstance)
    {
    }
    static inline Interface* newInstance(FeatureInstanceHandle hInst) { return new Interface(hInst); }
    inline FeatureInstanceHandle getHandle() const override { return _hInst; }
    dog* createDog(AppendData append_data, FtInt type) override;
    pigeon* createPigeon(AppendData append_data) override;
    cock* createCock(AppendData append_data) override;
    IAnimal* createCat(AppendData append_data) override;
    void setAnimal(AppendData append_data, ft_utils::RefPtr<IAnimal> animal) override;
    void flyFar(AppendData append_data, FtPromiseId pid, FtInt distance) override;
    void print(AppendData append_data, FtVariParams vari_params) override;
};

}