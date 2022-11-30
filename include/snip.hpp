//
// Created by maxng on 11/26/2022.
//

#ifndef SNIP_S2_HPP
#define SNIP_S2_HPP

#include <thread>
#include <iostream>
#include <chrono>
#include <utility>
#include <vector>
#include <utility>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <variant>
#include <optional>

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

/// Generate listener/dispatcher for events.
#define SNIP_DISPATCHER(listener, ...) struct listener##_dispatcher {                                                 \
                                           using listener_##listener##_signature = std::function<void (__VA_ARGS__)>; \
                                                                                                                      \
                                       protected:                                                                     \
                                           std::vector<listener_##listener##_signature> m_listeners;                  \
                                                                                                                      \
                                           template <typename... t_args>                                              \
                                           void dispatch_##listener(t_args&&... a_args) {                             \
                                               for (auto& listener : m_listeners) {                                   \
                                                   listener(std::forward<t_args>(a_args)...);                         \
                                               }                                                                      \
                                           }                                                                          \
                                                                                                                      \
                                       public:                                                                        \
                                           void on_##listener(listener_##listener##_signature a_function) {           \
                                               m_listeners.push_back(std::move(a_function));                          \
                                           }                                                                          \
                                       };                                                                             \

namespace snip {
    constexpr size_t version = 1;

    using io_context = boost::asio::io_context;
    using acceptor = boost::asio::ip::tcp::acceptor;
    using socket = boost::asio::ip::tcp::socket;
    using error_code = boost::system::error_code;
    using port_type = boost::asio::ip::port_type;
    using resolver = boost::asio::ip::tcp::resolver;

    /// Get the current system time in nanoseconds.
    [[nodiscard]] std::uint64_t system_nano_time() {
        return std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    /// Get the current system time in milliseconds.
    [[nodiscard]] std::uint64_t system_millis_time() {
        return system_nano_time() / 1000000;
    }

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

    /// Executes a console command and returns its output.
    std::string execute(const std::string& a_command) {
        std::array<char, 128> buffer{};
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(a_command.c_str(), "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

#ifdef _WIN32
#undef popen
#undef pclose
#endif

    /// String utility library.
    namespace string {
        /// Test if string ends with specified string.
        [[nodiscard]] bool ends_with(std::string_view a_string, std::string_view a_ends_with) {
            return a_string.size() >= a_ends_with.size()
            && a_string.substr(a_string.size() - a_ends_with.size(), a_ends_with.size()) == a_ends_with;
        }

        /// Test if string begins with specified string.
        [[nodiscard]] bool starts_with(std::string_view a_string, std::string_view a_starts_with) {
            return a_string.size() >= a_starts_with.size()
            && a_string.substr(0, a_starts_with.size()) == a_starts_with;
        }

        /// Removes whitespace from both sides of the string.
        [[nodiscard]] std::string_view trim(std::string_view a_string) {
            auto begin_it = std::find_if(a_string.begin(), a_string.end(), [](const auto ch) noexcept -> bool {
                return !std::isspace(ch);
            });

            auto end_it = std::find_if(a_string.rbegin(), a_string.rend(), [](const auto ch) noexcept -> bool {
                return !(std::isspace(ch) || std::iscntrl(ch));
            });

            const auto begin_difference = begin_it - a_string.cbegin();
            const auto end_difference = end_it - a_string.crbegin();

            return a_string.substr(begin_difference, a_string.length() - (begin_difference + end_difference));
        }

        [[nodiscard]] bool equals_ignore_case(std::string_view a_lhs, std::string_view a_rhs) {
            if (a_lhs.size() != a_rhs.size()) {
                return false;
            }

            for (size_t i = 0; i < a_lhs.size(); ++i) {
                if (std::tolower(a_lhs[i]) != std::tolower(a_rhs[i])) {
                    return false;
                }
            }

            return true;
        }
    }

    /// Check JSON value, return default parameter if JSON value is null, otherwise return JSON value.
    template <typename t_type>
    t_type json_default(nlohmann::json& a_json, const t_type& a_default) {
        if (a_json.is_null()) {
            return a_default;
        } else {
            return a_json.get<t_type>();
        }
    }

    enum class field_mode : size_t {
        input  = 0,
        output = 1,
        both   = 2
    };

    std::string_view field_mode_to_string(field_mode a_mode) {
        static constexpr std::string_view field_modes[] = {
            "input",
            "output",
            "both"
        };

        return field_modes[static_cast<size_t>(a_mode)];
    }

    field_mode string_to_field_mode(std::string_view a_mode) {
        if (a_mode == "input") {
            return field_mode::input;
        } else if (a_mode == "output") {
            return field_mode::output;
        } else {
            return field_mode::both;
        }
    }

    enum class field_type : size_t {
        boolean  = 0,
        integer  = 1,
        floating = 2,
        string   = 3
    };

    using field_boolean = bool;
    using field_integer = int64_t;
    using field_float   = double;
    using field_string  = std::string;

    std::string_view field_type_to_string(field_type a_type) {
        static constexpr std::string_view field_types[] = {
            "boolean",
            "integer",
            "float",
            "string"
        };

        return field_types[static_cast<size_t>(a_type)];
    }

    field_type string_to_field_type(std::string_view a_type) {
        if (a_type == "boolean") {
            return field_type::boolean;
        } else if (a_type == "integer") {
            return field_type::integer;
        } else if (a_type == "float") {
            return field_type::floating;
        } else {
            return field_type::string;
        }
    }

    struct field_schema {
        std::string id;  ///< Identifier of the field.
        field_mode mode; ///< Mode of the field (output, input, both).
        field_type type; ///< Type of the field (boolean, integer, etc.).
        bool reported;   /**< Whether the field is "reported" in periodic REPORT messages or needs to be explicitly
                          *   retrieved by a GET message. */

        nlohmann::json proprietary = nullptr; ///< Extra meta information about the field.

        std::string machine; ///< Machine managing the field. Unused by client.
    };

    using field_data = std::variant<field_boolean, field_integer, field_float, field_string>;

    struct field {
        field_schema schema;
        std::optional<field_data> value;

        template <typename t_type>
        std::optional<t_type> get_value() {
            if (value.has_value()) {
                return std::nullopt;
            }

            return std::get<t_type>(value.value());
        }

        std::optional<field_boolean> get_boolean() {
            if (!value.has_value() || schema.type != field_type::boolean) {
                return std::nullopt;
            }

            return std::get<field_boolean>(value.value());
        }

        std::optional<field_integer> get_integer() {
            if (value.has_value() || schema.type != field_type::integer) {
                return std::nullopt;
            }

            return std::get<field_integer>(value.value());
        }

        std::optional<field_float> get_float() {
            if (value.has_value() || schema.type != field_type::floating) {
                return std::nullopt;
            }

            return std::get<field_float>(value.value());
        }

        std::optional<std::string_view> get_string() {
            if (value.has_value() || schema.type != field_type::string) {
                return std::nullopt;
            }

            return std::get<field_string>(value.value());
        }

        nlohmann::json value_to_json() {
            switch (schema.type) {
                case field_type::boolean:
                    return std::get<field_boolean>(value.value());
                case field_type::integer:
                    return std::get<field_integer>(value.value());
                case field_type::floating:
                    return std::get<field_float>(value.value());
                case field_type::string:
                    return std::get<field_string>(value.value());
                default:
                    return nullptr;
            }
        }

        nlohmann::json to_json() {
            nlohmann::json val = {
                    { "mode", field_mode_to_string(schema.mode) },
                    { "type", field_type_to_string(schema.type) },
                    { "reported", schema.reported }
            };

            if (!schema.proprietary.is_null()) {
                val["proprietary"] = schema.proprietary;
            }

            return val;
        }
    };

    /**
     * Class for fields that are not reported (require explict GET messages to retrieve data) and fields that can
     * be set using a SET message.
     */
    struct field_handler {
        template <typename t_type>
        using signature_get = std::conditional_t<std::is_same_v<t_type, field_boolean>, std::function<field_boolean ()>,
                              std::conditional_t<std::is_same_v<t_type, field_integer>, std::function<field_integer ()>,
                              std::conditional_t<std::is_same_v<t_type, field_float  >, std::function<field_float   ()>,
                              std::conditional_t<std::is_same_v<t_type, field_string >, std::function<field_string  ()>,
                              void
                          >>>>;

        template <typename t_type>
        using signature_set = std::conditional_t<std::is_same_v<t_type, field_boolean>, std::function<void (field_boolean)>,
                              std::conditional_t<std::is_same_v<t_type, field_integer>, std::function<void (field_integer)>,
                              std::conditional_t<std::is_same_v<t_type, field_float  >, std::function<void (field_float)>,
                              std::conditional_t<std::is_same_v<t_type, field_string >, std::function<void (field_string)>,
                              void
                          >>>>;

        using get_pack = std::variant<
                signature_get<field_boolean>,
                signature_get<field_integer>,
                signature_get<field_float>,
                signature_get<field_string>
            >;

        using set_pack = std::variant<
                signature_set<field_boolean>,
                signature_set<field_integer>,
                signature_set<field_float>,
                signature_set<field_string>
            >;

        get_pack get_function;
        set_pack set_function;

        bool get_available = false;
        bool set_available = false;

        template <typename t_value>
        void set(t_value&& a_value) {
            std::get<signature_set<t_value>>(set_function)(std::forward<t_value>(a_value));
        }

        template <typename t_value>
        t_value get() {
            return std::get<signature_get<t_value>>(get_function)();
        }

        template <typename t_value>
        void on_set(signature_set<t_value> a_function) {
            set_available = true;
            set_function = a_function;
        }

        template <typename t_value>
        void on_get(signature_get<t_value> a_function) {
            get_available = true;
            get_function = a_function;
        }

        [[nodiscard]] bool getter_available() const noexcept {
            return get_available;
        }

        [[nodiscard]] bool setter_available() const noexcept {
            return set_available;
        }
    };

    /**
     * @brief Stores a SNIP message.
     *
     * Contains SNIP message information including method, headers, and body.
     */
    struct message {
        std::string method;                                   ///< SNIP message method.
        size_t version = ::snip::version;                     ///< SNIP version.
        std::unordered_map<std::string, std::string> headers; ///< SNIP message headers.
        std::string body;                                     ///< SNIP message body.

        /// Converts object to a SNIP message, ready to be sent across the network.
        std::string to_string() {
            std::string resp;
            resp += "SNIP/" + std::to_string(version) + ' ' + method + '\n';

            // Store content length of body. Don't trust users to define themselves.
            if (!body.empty()) {
                headers["Content-Length"] = std::to_string(body.size());
            }

            for (auto& header : headers) {
                resp += header.first + ": " + header.second + '\n';
            }

            resp += '\n';

            resp += body;

            return resp;
        }

        /// Sends a message through an active socket asynchronously while maintaining lifetime of the message.
        void send(socket& a_socket, const std::function<void (error_code ec, size_t)>& a_callback = [](error_code, size_t){}) {
            std::shared_ptr<std::string> str = std::make_shared<std::string>(to_string());
            boost::asio::async_write(a_socket, boost::asio::buffer(*str), [str, a_callback] (error_code ec, size_t bytes) {
                a_callback(ec, bytes);
            });
        }

        /// Respond to this message with another. Copies State header.
        void respond(socket& a_socket, message& a_msg) {
            auto state_it = headers.find("State");
            if (state_it != headers.cend()) {
                a_msg.headers["State"] = state_it->second;
            }

            auto tracking_it = headers.find("Proxy-Tracking-Identifier");
            if (tracking_it != headers.cend()) {
                a_msg.headers["Proxy-Tracking-Identifier"] = tracking_it->second;
            }

            auto response_it = headers.find("Response-Key");
            if (response_it != headers.cend()) {
                a_msg.headers["Response-Key"] = response_it->second;
            }

            a_msg.send(a_socket);
        }
    };

    enum class machine_type : size_t {
        none       = 0,
        monitor    = 1,
        controller = 2,
    };

    std::string_view machine_type_to_string(machine_type a_type) {
        static constexpr std::string_view machine_types[] = {
                "None",
                "Monitor",
                "Controller"
        };

        return machine_types[static_cast<size_t>(a_type)];
    }

    machine_type string_to_machine_type(std::string_view a_type) {
        if (a_type == "Monitor") {
            return machine_type::monitor;
        } else if (a_type == "Controller") {
            return machine_type::controller;
        } else {
            return machine_type::none;
        }
    }

    /**
     * @brief Stores information about a client (slave) connected to the server.
     */
    struct client_connection {
        /// Mutex to guarantee synchronous accessing of connection fields.
        std::mutex access_mutex;

        size_t id;                                      ///< Connection ID. Guaranteed to be unique for all connections.
        socket m_socket;                                ///< Client socket.
        std::shared_ptr<boost::asio::streambuf> buffer; ///< Buffer used for incoming messages in an asynchronous call.
        bool verified = false;                          ///< Whether the client has sent an IDENTIFY message.

        std::string machine_id;                 ///< The ID of the machine, if verified = true.
        machine_type type = machine_type::none; ///< The type of the machine (monitor/controller).

        std::atomic_size_t last_heartbeat = system_millis_time(); ///< Stores time of last heartbeat message in milliseconds.
        std::atomic_size_t last_notify = 0;                       ///< Stores time of last missed heartbeat notification.

        std::array<std::pair<size_t, std::function<void (message&)>>, 64> response_listeners;
        size_t response_index = 0;
        size_t response_counter = 0;

        client_connection(size_t a_id, socket&& a_socket) : id(a_id), m_socket(std::move(a_socket)), buffer(std::make_shared<boost::asio::streambuf>()) {}

        /**
         * @brief Sends a message to the client.
         * @param a_msg The message to send.
         * @param a_response_callback [optional] Include a callback for a response message.
         */
        void send_message(message& a_msg, const std::function<void (message&)>& a_response_callback = {}) {
            if (a_response_callback) {
                size_t key = response_counter++;

                // Emplace fields to track and provide callback for responses.
                a_msg.headers["Response-Key"] = std::to_string(key);

                if (response_index++ >= response_listeners.size()) {
                    response_index = 0;
                }

                response_listeners[0] = { key, a_response_callback };
            }

            a_msg.send(m_socket);
        }

        /**
         * @brief Send a command to a client to execute.
         * @param a_command The command to execute.
         * @param a_callback [optional] Callback for the command's output.
         */
        void execute(std::string a_command, const std::function<void (std::string_view)>& a_callback = {}) {
            message msg;
            msg.method = "EXECUTE";
            msg.body = std::move(a_command);

            if (a_callback) {
                send_message(msg, [a_callback](message& response) {
                    a_callback(response.body);
                });
            } else {
                send_message(msg);
            }
        }

        /**
         * @brief Send an information request to a client.
         * @param a_key The key of the information to retrieve.
         * @param a_callback [optional] Callback for the information value.
         */
        void information(std::string a_key, const std::function<void (std::string_view)>& a_callback = {}) {
            message msg;
            msg.method = "INFORMATION";
            msg.headers["Key"] = std::move(a_key);

            if (a_callback) {
                send_message(msg, [a_callback](message& response) {
                    a_callback(response.body);
                });
            } else {
                send_message(msg);
            }
        }

        /**
         * @brief Send a get request to a client.
         * @param a_key The key of the field to retrieve.
         * @param a_callback [optional] Callback for the field value.
         */
        void get(std::string a_key, const std::function<void (std::string_view)>& a_callback = {}) {
            message msg;
            msg.method = "GET";
            msg.headers["Key"] = std::move(a_key);

            if (a_callback) {
                send_message(msg, [a_callback](message& response) {
                    auto status_it = response.headers.find("Status");

                    if (status_it == response.headers.cend() || status_it->second != "Failure") {
                        a_callback(response.body);
                    }
                });
            } else {
                send_message(msg);
            }
        }

        /**
         * @brief Send a set request to a client.
         * @param a_key The key of the field to set.
         * @param a_value The value to which to set the field.
         * @param a_callback [optional] Callback for the field value.
         */
        template <typename t_value>
        void set(std::string a_key, const t_value& a_value, const std::function<void (message& msg)>& a_callback = {}) {
            message msg;
            msg.method = "SET";
            msg.headers["Key"] = std::move(a_key);

            if constexpr (std::is_convertible_v<t_value, std::string_view>) {
                msg.body = std::string(std::string_view(a_value));
            } else if constexpr (std::is_convertible_v<t_value, field_boolean>) {
                msg.body = field_boolean(a_value) ? "true" : "false";
            } else {
                msg.body = std::to_string(a_value);
            }

            send_message(msg, a_callback);
        }
    };

    /// Accept an incoming message and parse into a message object.
    bool digest_message(message& req, socket& a_socket, boost::asio::streambuf& a_buf);

    SNIP_DISPATCHER(client_connect, client_connection&);
    SNIP_DISPATCHER(client_message_receive, client_connection&, message&);
    SNIP_DISPATCHER(machine_register, client_connection&);
    SNIP_DISPATCHER(server_connect);
    SNIP_DISPATCHER(client_disconnect, client_connection&);
    SNIP_DISPATCHER(missed_heartbeat, client_connection&);
    SNIP_DISPATCHER(heartbeat_receive, client_connection&);
    SNIP_DISPATCHER(message_receive, message&);
    SNIP_DISPATCHER(schema_receive, client_connection&, nlohmann::json&);
    SNIP_DISPATCHER(report_receive, client_connection&, nlohmann::json&);

    /**
     * @brief SNIP server implementation.
     *
     * SNIP server implementation. Manages slaves, incoming connections, etc.
     */
    class server :
        public client_connect_dispatcher,
        public client_message_receive_dispatcher,
        public machine_register_dispatcher,
        public client_disconnect_dispatcher,
        public missed_heartbeat_dispatcher,
        public heartbeat_receive_dispatcher,
        public schema_receive_dispatcher,
        public report_receive_dispatcher
    {
        io_context m_context;
        acceptor m_acceptor;

        std::thread m_thread;       ///< Thead to manage incoming connections and messages.
        std::atomic_bool m_running; ///< Control boolean for thread.

        std::vector<std::unique_ptr<client_connection>> m_clients; ///< List of all connected clients.
        std::mutex m_clients_guard;                                ///< Guard to synchronize changes to client list.

        size_t m_id_counter = 0; ///< Current counter for assigning connection IDs. Increments on each new connection.


        size_t m_heartbeat_interval = 3000; /**< Required interval between heartbeats from clients. (milliseconds)
                                             *   Set to zero to disable. */

        size_t m_heartbeat_notify = 5000;   ///< Interval before a missed heartbeat is signaled. (milliseconds)

        size_t m_heartbeat_disconnect = 20000; /**< Required time by which to respond to a heartbeat or be
                                                *   disconnected. (milliseconds) */

        size_t m_tracking_counter = 0; ///< Current counter for assigning proxy tracking identifiers.

        std::map<std::string, field> m_fields;

    public:
        server(port_type a_port) : m_context(), m_acceptor(m_context,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), a_port)), m_running(false) {
        }

        /// Start server and network thread.
        void start() {
            m_running = true;
            m_thread = std::thread(&server::run, this);
        }

        /// Stop server and network thread.
        void stop() {
            m_running = false;
            m_thread.join();
        }

        void disconnect_client(client_connection& a_conn) {
            dispatch_client_disconnect(a_conn);

            a_conn.m_socket.cancel();

            std::lock_guard g2(m_clients_guard);
            m_clients.erase(std::remove_if(m_clients.begin(), m_clients.end(), [&a_conn](auto& a_client) -> bool {
                return a_client->id == a_conn.id;
            }), m_clients.end());
        }

        std::lock_guard<std::mutex> clients_list_guard() {
            return std::lock_guard<std::mutex>{ m_clients_guard };
        }

        auto& get_clients() {
            return m_clients;
        }

        auto& get_fields() {
            return m_fields;
        }

        template <typename t_type>
        std::optional<t_type> get_field_value(const std::string& a_key) {
            auto field_it = m_fields.find(a_key);

            if (field_it != m_fields.cend()) {
                return field_it->second.get_value<t_type>();
            } else {
                return std::nullopt;
            }
        }

        std::optional<field_boolean> get_field_boolean(const std::string& a_key) {
            auto field_it = m_fields.find(a_key);

            if (field_it != m_fields.cend()) {
                return field_it->second.get_boolean();
            } else {
                return std::nullopt;
            }
        }

        std::optional<field_integer> get_field_integer(const std::string& a_key) {
            auto field_it = m_fields.find(a_key);

            if (field_it != m_fields.cend()) {
                return field_it->second.get_integer();
            } else {
                return std::nullopt;
            }
        }

        std::optional<field_float> get_field_float(const std::string& a_key) {
            auto field_it = m_fields.find(a_key);

            if (field_it != m_fields.cend()) {
                return field_it->second.get_float();
            } else {
                return std::nullopt;
            }
        }

        std::optional<std::string_view> get_field_string(const std::string& a_key) {
            auto field_it = m_fields.find(a_key);

            if (field_it != m_fields.cend()) {
                return field_it->second.get_string();
            } else {
                return std::nullopt;
            }
        }

    private:
        void begin_accept() {
            m_acceptor.async_accept([this](error_code ec, socket sock) {
                if (!ec) {
                    handle_accept(ec, std::move(sock));
                }

                begin_accept();
            });
        }

        void handle_accept(error_code& a_ec, socket&& a_socket) {
            // Synchronize m_clients modification/accessing.
            std::lock_guard g(m_clients_guard);

            // Add new client with a unique ID.
            m_clients.emplace_back(std::make_unique<client_connection>(m_id_counter++, std::move(a_socket)));

            auto& conn = *m_clients.back();

            dispatch_client_connect(conn);

            // Send initial heartbeat message.
            if (m_heartbeat_interval > 0) {
                message msg;
                msg.method = "HEARTBEAT";
                msg.headers["Interval"] = std::to_string(m_heartbeat_interval);
                msg.headers["Disconnect-Time"] = std::to_string(m_heartbeat_disconnect);
                msg.send(conn.m_socket);
            }

            // Start listening for messages from the client.
            accept_message(conn);
        }

        void accept_message(client_connection& a_conn) {
            // Read until end of headers.
            boost::asio::async_read_until(a_conn.m_socket, *a_conn.buffer, "\n\n", [this, buffer_storage = a_conn.buffer, &a_conn](const error_code ec, const std::size_t bytes_transferred) {
                // Return if client connection has been terminated.
                if (std::find_if(m_clients.begin(), m_clients.end(), [&a_conn](auto& client) -> bool { return &*client == &a_conn; }) == m_clients.cend()) {
                    return;
                }

                if (ec == boost::asio::error::connection_reset) {
                    // Detected client disconnect.

                    disconnect_client(a_conn);
                    return;
                }

                // Handle incoming message and wait for new messages.
                std::lock_guard g(a_conn.access_mutex);
                handle_incoming_message(a_conn);
                accept_message(a_conn);
            });
        }

        void handle_incoming_message(client_connection& a_conn) {
            message req;

            if (digest_message(req, a_conn.m_socket, *a_conn.buffer)) {
                handle_message_receive(a_conn, req);
            }
        }

        void handle_message_receive(client_connection& a_client, message& a_msg) {
            auto proxy_target_it = a_msg.headers.find("Proxy-Target");
            auto proxy_tracking_it = a_msg.headers.find("Proxy-Tracking");

            if (proxy_target_it != a_msg.headers.cend()) {
                std::string response_key;
                auto response_key_it = a_msg.headers.find("Response-Key");

                if (response_key_it != a_msg.headers.cend()) {
                    response_key = response_key_it->second;
                }

                a_msg.headers["Proxy-Tracking"] = std::to_string(m_tracking_counter++);

                for (auto& client : m_clients) {
                    if (client->machine_id == proxy_target_it->second) {
                        if (response_key.empty()) {
                            client->send_message(a_msg);
                        } else {
                            client->send_message(a_msg, [&a_client, response_key](auto& msg) {
                                msg.headers["Response-Key"] = response_key;
                                msg.send(a_client.m_socket);
                            });
                        }

                        break;
                    }
                }
            } else if (proxy_tracking_it != a_msg.headers.cend()) {

            }
            // Dispatch response header to any client-specific response listeners.
            else if (string::ends_with(a_msg.method, "RESPONSE")) {
                auto key_it = a_msg.headers.find("Response-Key");

                if (key_it != a_msg.headers.cend()) {
                    size_t key = std::stoll(key_it->second);

                    auto listener_it = std::find_if(
                        a_client.response_listeners.begin(),
                        a_client.response_listeners.end(),
                        [key](auto& listener) -> bool {
                            return listener.first == key;
                        }
                    );

                    if (listener_it != a_client.response_listeners.cend()) {
                        listener_it->second(a_msg);
                    }
                }
            } else if (a_msg.method == "IDENTIFY") { // IDENTIFY message.
                auto machine_it = a_msg.headers.find("Machine");
                auto type_it = a_msg.headers.find("Machine-Type");

                if (machine_it != a_msg.headers.cend()) {
                    a_client.machine_id = machine_it->second;

                    if (type_it != a_msg.headers.cend()) {
                        a_client.type = string_to_machine_type(type_it->second);
                    } else {
                        a_client.type = machine_type::monitor;
                    }

                    a_client.verified = true;

                    dispatch_machine_register(a_client);
                }
            } else if (a_msg.method == "HEARTBEAT") {
                a_client.last_heartbeat = system_millis_time();
                dispatch_heartbeat_receive(a_client);
            } else if (a_msg.method == "INFORMATION") {
                handle_information_request(a_client, a_msg);
            } else if (a_msg.method == "SCHEMA") {
                nlohmann::json data = nlohmann::json::parse(a_msg.body);

                for (nlohmann::json::iterator it = data.begin(); it != data.end(); ++it) {
                    const std::string& field_id = it.key();
                    auto& field_schema = it.value();

                    if (field_schema["mode"].is_null() || field_schema["type"].is_null() || field_schema["reported"].is_null()) {
                        message resp;
                        resp.method = "SCHEMA-RESPONSE";resp.headers["Status"] = "Failure";
                        resp.headers["Status-Code"] = "MISSING_DATA";
                        resp.headers["Status-Message"] = "Specified fields are missing required information.";
                        resp.headers["Field"] = field_id;
                        a_msg.respond(a_client.m_socket, resp);
                        return;
                    }

                    m_fields[field_id] = {
                        {
                            field_id,
                            string_to_field_mode(field_schema["mode"].get<std::string_view>()),
                            string_to_field_type(field_schema["type"].get<std::string_view>()),
                            field_schema["reported"].get<bool>(),
                            field_schema["proprietary"],
                            a_client.machine_id
                        },
                        std::nullopt
                    };
                }

                dispatch_schema_receive(a_client, data);
            } else if (a_msg.method == "REPORT") {
                handle_report(a_client, a_msg);
            }

            dispatch_client_message_receive(a_client, a_msg);
        }

        void handle_report(client_connection& a_conn, message& a_msg) {
            nlohmann::json data = nlohmann::json::parse(a_msg.body);

            // Retrieve and store fields from message.
            for (nlohmann::json::iterator it = data.begin(); it != data.cend(); ++it) {
                const std::string& key = it.key();

                auto field_it = m_fields.find(key);

                if (field_it == m_fields.cend()) {
                    continue;
                }

                auto& value = it.value();

                switch (field_it->second.schema.type) {
                    case field_type::boolean:
                        if (value.is_boolean()) {
                            m_fields[key].value = field_boolean(value);
                        }

                        break;
                    case field_type::integer:
                        if (value.is_number_integer()) {
                            m_fields[key].value = field_integer(value);
                        }

                        break;
                    case field_type::floating:
                        if (value.is_number_float()) {
                            m_fields[key].value = field_float(value);
                        }

                        break;
                    case field_type::string:
                        if (value.is_string()) {
                            m_fields[key].value = field_string(value);
                        }

                        break;
                }
            }

            dispatch_report_receive(a_conn, data);
        }

        void handle_information_request(client_connection& a_conn, message& a_msg) {
            auto key_it = a_msg.headers.find("Key");

            message resp;
            resp.method = "INFORMATION-RESPONSE";

            if (key_it == a_msg.headers.cend()) {
                resp.headers["Status"] = "Failure";
                resp.headers["Status-Code"] = "MISSING_KEY";
                resp.headers["Status-Message"] = "Missing key value in request.";
                a_msg.respond(a_conn.m_socket, resp);
                return;
            }

            resp.headers["Key"] = key_it->second;

            if (key_it->second == "Machine-List") {
                auto client_it = m_clients.begin();

                // Find first verified client.
                while (client_it != m_clients.cend() && !(*client_it)->verified) {
                    ++client_it;
                }

                if (client_it != m_clients.cend()) {
                    std::string clients_list = (*client_it)->machine_id;

                    for (++client_it; client_it != m_clients.cend(); ++client_it) {
                        std::cout << (*client_it)->machine_id << std::endl;

                        clients_list += ", ";
                        clients_list += (*client_it)->machine_id;
                    }

                    resp.body = std::move(clients_list);
                }
            } else if (key_it->second == "Field-List") {
                nlohmann::json data;

                auto machine_it = a_msg.headers.find("Target-Machine");

                if (machine_it != a_msg.headers.cend()) {
                    resp.headers["Target-Machine"] = machine_it->second;

                    for (auto& field : m_fields) {
                        if (field.second.schema.machine == machine_it->second) {
                            data[field.second.schema.id] = field.second.to_json();
                        }
                    }
                } else {
                    for (auto& field : m_fields) {
                        data[field.second.schema.id] = field.second.to_json();
                    }
                }

                resp.body = to_string(data);
            } else if (key_it->second == "Value-List") {
                nlohmann::json data;

                auto machine_it = a_msg.headers.find("Target-Machine");

                if (machine_it != a_msg.headers.cend()) {
                    resp.headers["Target-Machine"] = machine_it->second;

                    for (auto& field : m_fields) {
                        if (field.second.value.has_value() && field.second.schema.machine == machine_it->second) {
                            data[field.second.schema.id] = field.second.value_to_json();
                        }
                    }
                } else {
                    for (auto& field : m_fields) {
                        if (field.second.value.has_value()) {
                            data[field.second.schema.id] = field.second.value_to_json();
                        }
                    }
                }

                resp.body = to_string(data);
            }

            a_msg.respond(a_conn.m_socket, resp);
        }

        void run() {
            begin_accept();

            while (m_running) {
                m_context.poll_one();

                auto current_time_millis = system_millis_time();

                for (auto& client : m_clients) {
                    auto time_since_last_heartbeat = current_time_millis - client->last_heartbeat;

                    if (time_since_last_heartbeat > m_heartbeat_disconnect) {
                        disconnect_client(*client);
                    } else if (time_since_last_heartbeat > m_heartbeat_notify) {
                        if (current_time_millis - client->last_notify > m_heartbeat_notify) {
                            client->last_notify = current_time_millis;

                            dispatch_missed_heartbeat(*client);
                        }
                    }
                }
            }
        }
    };

    class client :
        public server_connect_dispatcher,
        public message_receive_dispatcher
    {
        io_context m_io_context;
        resolver m_resolver;
        socket m_socket;
        bool m_connected = false;

        boost::asio::streambuf m_buffer;

        std::string m_machine_id;
        machine_type m_type;

        std::thread m_thread;
        std::atomic_bool m_running = false;

        size_t m_heartbeat_interval = 0;

        std::array<std::pair<size_t, std::function<void (message&)>>, 64> m_response_listeners;
        size_t m_response_index = 0;
        size_t m_response_counter = 0;

        std::map<std::string, field> m_fields;
        std::map<std::string, field_handler> m_field_handlers;

    public:
        explicit client(std::string a_machine_id, machine_type a_type) : m_io_context(), m_resolver(m_io_context),
            m_socket(m_io_context), m_machine_id(std::move(a_machine_id)), m_type(a_type) {}

        void start(const std::string& a_hostname, port_type a_port) {
            error_code ec;
            boost::asio::connect(m_socket, m_resolver.resolve(a_hostname, std::to_string(a_port)), ec);
            dispatch_server_connect();

            m_running = true;
            m_thread = std::thread(&client::run, this);

            m_connected = true;
        }

        void stop() {
            m_running = false;
            m_thread.join();
        }

        void send_message(message& a_msg, const std::function<void (message&)>& a_response_callback = {}) {
            if (a_response_callback) {
                size_t key = m_response_counter++;

                // Emplace fields to track and provide callback for responses.
                a_msg.headers["Response-Key"] = std::to_string(key);

                if (m_response_index++ >= m_response_listeners.size()) {
                    m_response_index = 0;
                }

                m_response_listeners[0] = { key, a_response_callback };
            }

            a_msg.send(m_socket);
        }

        /**
         * @brief Send an information request to the server.
         * @param a_command The key of the information to retrieve.
         * @param a_callback [optional] Callback for the information value.
         */
        void information(std::string a_key, const std::function<void (std::string_view)>& a_callback = {}) {
            message msg;
            msg.method = "INFORMATION";
            msg.headers["Key"] = std::move(a_key);

            if (a_callback) {
                send_message(msg, [a_callback](message& response) {
                    a_callback(response.body);
                });
            } else {
                send_message(msg);
            }
        }

        field& add_field(field_schema a_field) {
            std::string id = a_field.id;
            return m_fields.emplace(std::move(id), field{ std::move(a_field), std::nullopt }).first->second;
        }

        field& get_field(const std::string& a_name) {
            return m_fields[a_name];
        }

        void register_field_handler(std::string a_name, field_handler a_handler) {
            m_field_handlers.emplace(std::move(a_name), std::move(a_handler));
        }

        /// Send field schema to server.
        void report_schema() {
            nlohmann::json schema_json;

            for (auto& field_pair : m_fields) {
                schema_json[field_pair.second.schema.id] = field_pair.second.to_json();
            }

            message msg;
            msg.method = "SCHEMA";
            msg.headers["Content-Type"] = "JSON";
            msg.body = to_string(schema_json);
            send_message(msg);
        }

        template <typename t_value>
        void set_field(const std::string& a_field, t_value a_value) {
            auto field_it = m_fields.find(a_field);

            if (field_it == m_fields.cend()) {
                return;
            }

            switch (field_it->second.schema.type) {
                case field_type::boolean:
                    if constexpr (std::is_convertible_v<t_value, field_boolean>) {
                        field_it->second.value = static_cast<field_boolean>(a_value);
                    }
                    else if constexpr (std::is_convertible_v<t_value, std::string_view>) {
                        field_it->second.value = string::equals_ignore_case(std::string_view(a_value), "true");
                    }
                    else if constexpr (std::is_convertible_v<t_value, field_integer>) {
                        field_it->second.value = static_cast<field_integer>(a_value) != 0;
                    }

                    break;
                case field_type::integer:
                    if constexpr (std::is_convertible_v<t_value, field_integer>) {
                        field_it->second.value = static_cast<field_integer>(a_value);
                    }
                    else if constexpr (std::is_convertible_v<t_value, std::string_view>) {
                        field_it->second.value = std::stoll(std::string(a_value));
                    }
                    else if constexpr (std::is_convertible_v<t_value, field_boolean>) {
                        field_it->second.value = static_cast<field_integer>(static_cast<field_boolean>(a_value));
                    }

                    break;
                case field_type::floating:
                    if constexpr (std::is_convertible_v<t_value, field_float>) {
                        field_it->second.value = static_cast<field_float>(a_value);
                    }
                    else if constexpr (std::is_convertible_v<t_value, std::string_view>) {
                        field_it->second.value = std::stod(std::string(a_value));
                    }
                    else if constexpr (std::is_convertible_v<t_value, field_boolean>) {
                        field_it->second.value = static_cast<field_float>(static_cast<field_boolean>(a_value));
                    }

                    break;
                case field_type::string:
                    if constexpr (std::is_convertible_v<t_value, std::string_view>) {
                        field_it->second.value = std::string(static_cast<std::string_view>(a_value));
                    }
                    else if constexpr (std::is_same_v<t_value, field_boolean>) {
                        field_it->second.value = std::string(static_cast<field_boolean>(a_value) ? "true" : "false");
                    }
                    else if constexpr (std::is_convertible_v<t_value, field_float>) {
                        field_it->second.value = std::to_string(static_cast<field_float>(a_value));
                    }

                    break;
            }
        }

        void report() {
            nlohmann::json data;

            for (auto& field : m_fields) {
                if (field.second.schema.reported) {
                    if (field.second.value.has_value()) {
                        // FSO
                        data[field.second.schema.id] = field.second.value_to_json();
                    }
                }
            }

            message msg;
            msg.method = "REPORT";
            msg.body = to_string(data);
            msg.send(m_socket);
        }

    private:
        void begin_accept() {
            boost::asio::async_read_until(m_socket, m_buffer, "\n\n", [this](const error_code ec, const std::size_t bytes_transferred) {
                handle_message();
            });
        }

        void handle_message() {
            message msg;

            if (digest_message(msg, m_socket, m_buffer)) {
                // Handle response messages.
                if (string::ends_with(msg.method, "RESPONSE")) {
                    auto key_it = msg.headers.find("Response-Key");

                    if (key_it != msg.headers.cend()) {
                        size_t key = std::stoll(key_it->second);

                        auto listener_it = std::find_if(
                            m_response_listeners.begin(),
                            m_response_listeners.end(),
                            [key](auto& listener) -> bool {
                                return listener.first == key;
                            }
                        );

                        if (listener_it != m_response_listeners.cend()) {
                            listener_it->second(msg);
                        }
                    }
                }

                if (msg.method == "HEARTBEAT") {
                    auto it = msg.headers.find("Interval");

                    if (it != msg.headers.cend()) {
                        m_heartbeat_interval = std::stoll(it->second);
                    }
                } else if (msg.method == "EXECUTE") {
                    // Start accepting a new message while commands
                    dispatch_message_receive(msg);
                    begin_accept();

                    message exec_response;
                    exec_response.method = "EXECUTE-RESPONSE";
                    exec_response.body = execute(msg.body);
                    msg.respond(m_socket, exec_response);

                    return;
                } else if (msg.method == "INFORMATION") {
                    handle_information_request(msg);
                } else if (msg.method == "GET") {
                    message resp;
                    resp.method = "GET-RESPONSE";

                    auto field_it = msg.headers.find("Key");

                    if (field_it != msg.headers.cend()) {
                        auto field_data_it = m_fields.find(field_it->second);
                        auto handler_it = m_field_handlers.find(field_it->second);

                        if (field_data_it == m_fields.cend()) {
                            resp.headers["Status"] = "Failure";
                            resp.headers["Status-Code"] = "MISSING_FIELD";
                            resp.headers["Status-Message"] = "Specified field does not exist on this machine.";
                        } else if (handler_it == m_field_handlers.cend()) {
                            resp.headers["Status"] = "Failure";
                            resp.headers["Status-Code"] = "MISSING_HANDLER";
                            resp.headers["Status-Message"] = "Specified field does not have an attached handler.";
                        } else {
                            if (handler_it->second.getter_available()) {
                                switch (field_data_it->second.schema.type) {
                                    case field_type::boolean:
                                        resp.body = handler_it->second.get<field_boolean>() ? "true" : "false";
                                        break;
                                    case field_type::integer:
                                        resp.body = std::to_string(handler_it->second.get<field_integer>());
                                        break;
                                    case field_type::floating:
                                        resp.body = std::to_string(handler_it->second.get<field_float>());
                                        break;
                                    case field_type::string:
                                        resp.body = handler_it->second.get<field_string>();
                                        break;
                                }
                            } else {
                                resp.headers["Status"] = "Failure";
                                resp.headers["Status-Code"] = "MISSING_GETTER";
                                resp.headers["Status-Message"] = "Specified field does not have a 'get' routine.";
                            }
                        }
                    } else {
                        resp.headers["Status"] = "Failure";
                        resp.headers["Status-Code"] = "MISSING_KEY";
                        resp.headers["Status-Message"] = "Missing field key.";
                    }

                    msg.respond(m_socket, resp);
                } else if (msg.method == "SET") {
                    message resp;
                    resp.method = "SET-RESPONSE";

                    auto field_it = msg.headers.find("Key");

                    if (field_it != msg.headers.cend()) {
                        auto field_data_it = m_fields.find(field_it->second);
                        auto handler_it = m_field_handlers.find(field_it->second);

                        if (field_data_it == m_fields.cend()) {
                            resp.headers["Status"] = "Failure";
                            resp.headers["Status-Code"] = "MISSING_FIELD";
                            resp.headers["Status-Message"] = "Specified field does not exist on this machine.";
                        } else if (handler_it == m_field_handlers.cend()) {
                            resp.headers["Status"] = "Failure";
                            resp.headers["Status-Code"] = "MISSING_HANDLER";
                            resp.headers["Status-Message"] = "Specified field does not have an attached handler.";
                        } else {
                            switch (field_data_it->second.schema.type) {
                                case field_type::boolean:
                                    handler_it->second.set<field_boolean>(string::equals_ignore_case(msg.body, "true"));
                                    break;
                                case field_type::integer:
                                    handler_it->second.set<field_integer>(std::stoll(msg.body));
                                    break;
                                case field_type::floating:
                                    handler_it->second.set<field_float>(std::stod(msg.body));
                                    break;
                                case field_type::string:
                                    handler_it->second.set<field_string>(std::string(msg.body));
                                    break;
                            }

                            resp.headers["Status"] = "Success";
                        }
                    } else {
                        resp.headers["Status"] = "Failure";
                        resp.headers["Status-Code"] = "MISSING_KEY";
                        resp.headers["Status-Message"] = "Missing field key.";
                    }

                    msg.respond(m_socket, resp);
                }

                dispatch_message_receive(msg);
            }

            begin_accept();
        }

        void handle_information_request(message& a_msg) {
            auto key_it = a_msg.headers.find("Key");

            message resp;
            resp.method = "INFORMATION-RESPONSE";

            if (key_it == a_msg.headers.cend()) {
                resp.headers["Status"] = "Failure";
                resp.headers["Status-Code"] = "MISSING_KEY";
                resp.headers["Status-Message"] = "Missing key value in request.";
                a_msg.respond(m_socket, resp);
                return;
            }

            resp.headers["Key"] = key_it->second;

            if (key_it->second == "Test") {
                resp.body = "Test Response";
            }

            a_msg.respond(m_socket, resp);
        }

        void run() {
            begin_accept();

            message identify_msg;
            identify_msg.method = "IDENTIFY";
            identify_msg.headers["Machine"] = m_machine_id;
            identify_msg.headers["Machine-Type"] = machine_type_to_string(m_type);
            identify_msg.send(m_socket);

            size_t last_heartbeat = system_millis_time();

            while (m_running) {
                m_io_context.poll_one();

                size_t current_time_millis = system_millis_time();

                if (m_heartbeat_interval > 0) {
                    if (current_time_millis - last_heartbeat > m_heartbeat_interval) {
                        last_heartbeat = current_time_millis;

                        message heartbeat_msg;
                        heartbeat_msg.method = "HEARTBEAT";
                        heartbeat_msg.send(m_socket);
                    }
                }
            }
        }
    };

    bool digest_message(message& req, socket& a_socket, boost::asio::streambuf& a_buffer) {
        std::istream header_stream(&a_buffer);

        std::string line;
        std::getline(header_stream, line);

        std::string_view top_view(line);

        if (top_view.substr(0, 5) != "SNIP/") {
            return false;
        }

        auto top_delim_it = std::find(line.begin(), line.end(), ' ');

        req.version = std::stoll(std::string(line.begin() + 5, top_delim_it));
        req.method = std::string(top_delim_it + 1, line.end());

        while (std::getline(header_stream, line) && !line.empty()) {
            auto header_delim_it = std::find(line.begin(), line.end(), ':');

            std::string key{ line.begin(), header_delim_it };
            std::string value{ header_delim_it + 1, line.end() };

            req.headers.emplace(std::move(key), std::string(string::trim(value)));
        }

        auto length_it = req.headers.find("Content-Length");
        if (length_it != req.headers.cend()) {
            size_t full_size = std::stoll(length_it->second);

            std::stringstream sstr;
            sstr << header_stream.rdbuf();
            req.body = sstr.str();

            if (req.body.length() < std::stoll(length_it->second)) {
                std::string remaining;
                remaining.resize(full_size - req.body.size());
                boost::asio::read(a_socket, boost::asio::buffer(remaining));
                req.body += remaining;
            }
        }

        return true;
    }
}

#endif //SNIP_S2_HPP
