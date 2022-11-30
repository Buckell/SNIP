#include <iostream>

#include <snip.hpp>

void wait(std::uint64_t time) {
    auto start = snip::system_millis_time();
    while (snip::system_millis_time() - start < time);
}

int main() {
    snip::client client("TestMachine", snip::machine_type::monitor);

    client.on_message_receive([](auto& msg){
        std::cout << "MSG: \n" << msg.to_string() << std::endl;
    });

    wait(1000);

    client.start("localhost", 55501);

    wait(5000);

    client.add_field({
        "pump1.power",
        snip::field_mode::both,
        snip::field_type::boolean,
        true
    });

    client.add_field({
         "pump1.power.draw",
         snip::field_mode::output,
         snip::field_type::floating,
         false,
         {
             { "name", "Power Draw" },
             { "unit", {
                 { "name", "Watt" },
                 { "abbr", "W" }
             } }
         }
    });

    snip::field_handler draw_handler;
    draw_handler.on_get<snip::field_float>([]() -> snip::field_float {
        std::cout << "GET POWER DRAW" << std::endl;
        return 500;
    });
    client.register_field_handler("pump1.power.draw", draw_handler);

    snip::field_handler power_handler;
    power_handler.on_set<snip::field_boolean>([&client](snip::field_boolean val) {
        std::cout << "SET POWER " << val << std::endl;
        client.set_field("pump1.power", val);
        client.report();
    });
    client.register_field_handler("pump1.power", power_handler);

    client.add_field({ "dummy.1", snip::field_mode::both, snip::field_type::boolean,  true });
    client.add_field({ "dummy.2", snip::field_mode::both, snip::field_type::integer,  true });
    client.add_field({ "dummy.3", snip::field_mode::both, snip::field_type::floating, true });
    client.add_field({ "dummy.4", snip::field_mode::both, snip::field_type::string,   true });

    client.report_schema();

    std::string cmd;
    while (std::getline(std::cin, cmd)) {
        if (cmd == "info") {
            client.information("Machine-List", [](auto msg) {
                std::cout << msg << std::endl;
            });
        } else if (snip::string::starts_with(cmd, "report")) {
            client.set_field("pump1.power", "false");

            client.set_field("dummy.1", true);
            client.set_field("dummy.2", 42);
            client.set_field("dummy.3", 0.05);
            client.set_field("dummy.4", "Hello, world!");
            client.report();
        }
    }

    return 0;
}
