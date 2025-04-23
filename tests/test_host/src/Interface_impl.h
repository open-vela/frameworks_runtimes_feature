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
    cock(FeatureInstanceHandle hInstance);
    virtual ~cock();
    // IAnimal
    ft_utils::FtStringPtr name() const override;
    void set_name(const ft_utils::FtStringPtr& val) override;
    FtInt legCount() const override;
    FtInt eatFood(const ft_utils::RefPtr<FtArray>& food) override;
    ft_utils::FtStringPtr run(FtInt distance, const ft_utils::FtStringPtr& destination) override;
    // IBird
    ft_utils::RefPtr<FtArray> fly() override;
    ft_utils::FtStringPtr breed() const override;
    void set_breed(const ft_utils::FtStringPtr& breed) override;
    // IChicken
    FtInt weight() const override;
    void set_weight(FtInt weight) override;
    void walk(FtPromiseId pid) override;
};

class dog : public IAnimal {
private:
    FtInt _type = 0;
    ft_utils::FtStringPtr _name;
    FtInt _legCount = 4;
    FtInt _eatFood = 100;

public:
    dog(FeatureInstanceHandle hInstance, FtInt type);
    virtual ~dog();
    ft_utils::FtStringPtr name() const override;
    void set_name(const ft_utils::FtStringPtr& val) override;
    FtInt legCount() const override;
    FtInt eatFood(const ft_utils::RefPtr<FtArray>& food) override;
    virtual ft_utils::FtStringPtr run(FtInt distance, const ft_utils::FtStringPtr& destination) override;
};

class cat : public dog {
public:
    cat(FeatureInstanceHandle hInstance);
    ft_utils::FtStringPtr run(FtInt distance, const ft_utils::FtStringPtr& destination) override;
};

class pigeon : public IBird {
private:
    ft_utils::FtStringPtr _breed;

public:
    pigeon(FeatureInstanceHandle hInstance)
        : IBird(hInstance)
    {
    }
    virtual ~pigeon();
    ft_utils::RefPtr<FtArray> fly() override;
    ft_utils::FtStringPtr breed() const override;
    void set_breed(const ft_utils::FtStringPtr& breed) override;
};

class Interface : public InterfaceBase {
private:
public:
    Interface(FeatureInstanceHandle hInstance)
        : InterfaceBase(hInstance)
    {
    }
    static inline Interface* Create(FeatureInstanceHandle hInst) { return new Interface(hInst); }
    IAnimal* createDog(FtInt type) override;
    IBird* createPigeon() override;
    IChicken* createCock() override;
    IAnimal* createCat() override;
    void setAnimal(const IAnimal*& animal) override;
    void flyFar(FtPromiseId pid, FtInt distance) override;
    void print(FtVariParams vari_params) override;
};

}