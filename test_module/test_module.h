#pragma once

#include "config.h"
#include "module.h"


class TestModule final : public IModule
{
public:
    TestModule(){}
    void pre_setup() override;
    void setup() override;
    void loop() override;
};
