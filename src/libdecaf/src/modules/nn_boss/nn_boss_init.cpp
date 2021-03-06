#include "nn_boss.h"
#include "nn_boss_init.h"

namespace nn
{

namespace boss
{

static bool
gInitialised = false;

nn::Result
Initialize()
{
   gInitialised = true;
   return nn::Result::Success;
}

nn::Result
Finalize()
{
   gInitialised = false;
   return nn::Result::Success;
}

bool
IsInitialized()
{
   return gInitialised;
}

void
Module::registerInitFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn4bossFv", nn::boss::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn4bossFv", nn::boss::Finalize);
   RegisterKernelFunctionName("IsInitialized__Q2_2nn4bossFv", nn::boss::IsInitialized);
}

}  // namespace boss

}  // namespace nn
