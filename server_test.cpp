#include <iostream>

#include <snip.hpp>

int main() {
    snip::server server(55501);

    server.on_client_connect([](snip::client_connection& conn) {
        std::cout << "CLIENT CONN: " << conn.id << std::endl;
    });

    server.on_client_message_receive([](snip::client_connection& conn, snip::message& msg) {
        if (msg.method == "HEARTBEAT" || snip::string::ends_with(msg.method, "RESPONSE")) {
            return;
        }

        std::cout << "CLIENT MSG RECEIVED: " << conn.id << std::endl;
        std::cout << msg.to_string() << std::endl;
    });

    server.on_machine_register([](snip::client_connection& conn) {
        std::cout << "CLIENT " << conn.id << " REGISTERED AS MACHINE " << conn.machine_id << std::endl;
    });

    server.on_client_disconnect([](snip::client_connection& conn) {
        std::cout << "CLIENT DISCONN: " << conn.id << std::endl;
    });

    server.on_missed_heartbeat([](auto& conn) {
        std::cout << "MISSED HEARTBEAT " << conn.id << std::endl;
    });

    server.on_heartbeat_receive([](auto& conn) {
        std::cout << "HEARTBEAT RECEIVED " << conn.id << std::endl;
    });

    server.start();


    std::string str;
    while (std::getline(std::cin, str)) {
        if (snip::string::starts_with(str, "execute ")) {
            auto& client = *server.get_clients()[0];

            client.execute(str.substr(8), [](std::string_view response) {
                std::cout << "CLIENT EXEC RESPONSE: \n" << response.substr(0, 100) << std::endl;
            });
        } else if (str == "info") {
            auto& client = *server.get_clients()[0];

            client.information("Test", [](auto val) {
                std::cout << "Test: " << val << std::endl;
            });
        } else if (str == "fields") {
            for (auto& field : server.get_fields()) {
                std::cout << "FIELD " << field.second.schema.id << ": "
                << field.second.schema.machine << " "
                << field_mode_to_string(field.second.schema.mode) << " "
                << field_type_to_string(field.second.schema.type) << " "
                << field.second.schema.reported << std::endl;

                if (field.second.value.has_value()) {
                    switch (field.second.schema.type) {
                        case snip::field_type::boolean:
                            std::cout << "VAL: " << std::get<snip::field_boolean>(field.second.value.value()) << std::endl;
                            break;
                        case snip::field_type::integer:
                            std::cout << "VAL: " << std::get<snip::field_integer>(field.second.value.value()) << std::endl;
                            break;
                        case snip::field_type::floating:
                            std::cout << "VAL: " << std::get<snip::field_float>(field.second.value.value()) << std::endl;
                            break;
                        case snip::field_type::string:
                            std::cout << "VAL: " << std::get<snip::field_string>(field.second.value.value()) << std::endl;
                            break;
                    }
                } else {
                    std::cout << "VAL: NULL" << std::endl;
                }
            }
        } else if (str == "get") {
            auto& client = *server.get_clients()[0];

            client.get("pump1.power.draw", [](auto val) {
                std::cout << "GET POWER DRAW " << val << std::endl;
            });
        } else if (snip::string::starts_with(str, "set ")) {
            auto& client = *server.get_clients()[0];
            bool power = std::stoll(str.substr(4)) != 0;

            client.set("pump1.power", power, [](auto& msg) {
                std::cout << "SET" << std::endl;
            });
        }
    }

    return 0;
}
