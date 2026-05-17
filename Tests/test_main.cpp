#include <catch2/catch_test_macros.hpp>
#include "PluginProcessor.h"

TEST_CASE("Plugin name is Grano", "[plugin]")
{
    GranoAudioProcessor proc;
    REQUIRE(proc.getName() == "Grano");
}
