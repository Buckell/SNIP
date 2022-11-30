#include <iostream>

#include <snip.hpp>

void wait(std::uint64_t time) {
    auto start = snip::system_millis_time();
    while (snip::system_millis_time() - start < time);
}

int main() {
    snip::client client("OtherMachine", snip::machine_type::controller);

    client.on_message_receive([](auto& msg){
        std::cout << "MSG: \n" << msg.to_string() << std::endl;
    });

    wait(1000);

    client.start("localhost", 55501);

    wait(5000);

    std::string cmd;
    while (std::getline(std::cin, cmd)) {
        if (cmd == "fields") {
            client.information("Field-List", [](std::string_view fields) {
                nlohmann::json data = nlohmann::json::parse(fields);

                std::cout << fields << std::endl;
            });
        } else if (cmd == "values") {
            client.information("Value-List", [](std::string_view fields) {
                nlohmann::json data = nlohmann::json::parse(fields);
                std::cout << fields << std::endl;
            });
        } else if (cmd == "proxy") {
            snip::message msg;
            msg.method = "EXECUTE";
            msg.headers["Proxy-Target"] = "TestMachine";
            msg.body = "ipconfig";
            client.send_message(msg, [](auto& msg) {
                std::cout << msg.to_string() << std::endl;
            });
        }
    }

    return 0;
}
