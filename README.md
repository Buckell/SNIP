# Systems Network Intra-Communication Protocol (SNIP)

The Systems Network Intra-Communication Protocol (SNIP) is an HTTP-like designed to allow a closed network of data 
monitoring and control devices to be managed by a single master device/server. These controlled devices, or slaves, 
can collect and report physical data, such as the water level in a tank or the ambient temperature of its location, or 
control physical elements, such as a set of lights or a door. SNIP offers a way for these devices to report collected 
data to the master server and allow the master server to control them and the physical elements they control.

In addition to "monitors," the devices collecting information or controlling physical elements, there are also another
type of slave device called "controllers." Controllers can act on behalf of the master server to read data and control
the monitor devices.

A common application of a protocol like this is in a smart home system. Devices can be connected to the network to 
control lights, heating or air conditioning, media devices, and more. In addition, the devices can also monitor things 
like whether windows/doors are open/closed, temperature, if lights or other devices have been left on, and more.

### Schema/Version 1

✅ - Denotes implemented features.

---
## Machine Types

### Master
The master is the host server to which all slave nodes communicate. The master handles all requests/responses from the
nodes and keeps track of information, states, and more.

### Slave
A slave is any device connected to the host server.

**Monitor** - Most slaves will be known as "Monitors," responsible for providing data to the master and handling
the direct interface to any subsystems managed by the network.

**Controller** - Other slaves will fall under the "Controller" category. Controllers are slaves that may act with
the authority of the master, being able to send requests to and query information from slave nodes.

---
### Method Types
There are two types of methods, those intended to be sent to a slave by the master, and those intended to be sent to
the master by the slave. The first kind, master to slave, will be denoted **MTS**. The second kind, slave to master,
will be denoted **STM**.

**Note:** Response versions of methods have `-RESPONSE` appended to the method name. For example, the response method
name to `EXECUTE` is `EXECUTE-RESPONSE`. The `State` header may be used to link a response to its original request.

---
## Miscellaneous Headers

### `State` ✅
Information stored in this header will be returned by the response.

### `Response-Key` ✅
Key send in a request to link a response to the request. Returning by the response.

### `Content-Length` ✅
The length (in bytes) of the message body.

### `Content-Type`
The type of the content in the message body. Accepted values include `Text`, `JSON`, and`CSV`. The default is `Text`.

### `Proxy-Target` ✅
When a Controller sends an MTS request to the master with the intention of forwarding that request to a Monitor node,
this header is included to specify the target machine of the request. MTS requests originating from a Controller must
include this header to specify to which machine to forward the request, otherwise it will be discarded. The inclusion
of this header will automatically have the master forward the request to the specified machine.

### `Proxy-Tracking` ✅
The identifier of the Controller that originated the proxy request. Used to track proxy requests and return the
response to the original proxy ma

### `Request-Content-Type`
Request a certain type to be returned when sending a request.

### `Status` ✅
A header sent in a response to indicate the successfulness/status of the request. Values can be `Success`, `Warning`,
or `Failure`.

### `Status-Code` ✅
A specific code that relates to the status of the previous request. Can be a number or string.

### `Status-Message` ✅
The message associated with the status returned by a request. Usually only used with `Status: Failure`.

---

## Methods

### `IDENTIFY` ✅

`STM`
Sends information to the master to identify a slave node.
```
SNIP/1 IDENTIFY
Machine: <Machine Name>
[Machine-Type: <Monitor | Controller>]        // Default: Monitor
```

### `HEARTBEAT` ✅

`MTS`
Sends a request to the slave for a heartbeat. This is a signal sent periodically to ensure a slave stays connected.
```
SNIP/1 HEARTBEAT
Interval: <Interval (ms)>     // Interval between heartbeat checks.
Disconnect-Time: <Time (ms)>  // Required time by which to respond or be disconnected.
```
`STM`
Sends a heartbeat to the server to signal an active/alive state.
```
SNIP/1 HEARTBEAT
```

### `EXECUTE` ✅
`MTS`
Sends a command to a slave node for execution.
```
SNIP/1 EXECUTE
<Command>
```

`STM`
The output of the command when the command's execution ends.
```
SNIP/1 EXECUTE-RESPONSE
<Command Output>
```

### `INFORMATION` ✅
`Both`
Requests information from the master/slave. Valid values for the `Key` header may be found in the proceeding section.
```
SNIP/1 INFORMATION
Key: <Key>
```
For example:
```
SNIP/1 INFORMATION
Key: Machine-List
```
`STM`
The requested information.
```
SNIP/1 INFORMATION-RESPONSE
Key: <Key>
<Information>
```

### `SCHEMA` ✅
`STM`
Reports a list/structure of fields that are reported or can be controlled in JSON format. The field_schema identifier is used
as keys to the JSON object, with field_schema information as the value.

#### Fields:
- `mode`:  Either `input`, `output`, or `both`. `input` denotes a value that may be written, `output` denotes a value
  that may be read
- `type`: The type of the field_schema, either `boolean`, `integer`, `float`, or `string`.
- `reported`: Whether the field_schema is periodically reported in `REPORT` messages.
- `proprietary`: Extra field_schema information that has no bearing on SNIP functionality but may be used to signal additional
  information to the master about the field_schema.

```
SNIP/1 SCHEMA
Content-Type: JSON
<Body>
```
For example:
```
SNIP/1 SCHEMA
Content-Type: JSON
{
    "machine.power": {                  // Machine on or off.
        "mode": "both",                 // Mode of field_schema. (input, output, both).
        "type": "boolean",              // Type of field_schema.
        "reported": true                // Reported periodically in REPORT messages.
    },
    "machine.power.draw": {
        "mode": "output",               // Read-only.
        "type": "float",
        "reported": true
    },
    "machine.flowrate": {
        "mode": "output",
        "type": "float",
        "reported": false,              // Must explicitly retrieve with a GET message.
        "proprietary": {
            "name": "Pump Flowrate",
            "description": "Volume of water flowing through the pump.",
            "unit": "Gallons Per Hour",
            "unit_abbreviation": "GPH"
        }
    }
}
```

### `REPORT` ✅
`STM`
Sends a periodic report of data to the master. All "output" values listed in a `SCHEMA` message and marked as
"reported" will be included in this report.
```
SNIP/1 REPORT
<Body>
```
For example:
```
SNIP/1 REPORT
{
    "machine.power": true,
    "machine.power.draw": 23.4,
    "machine.flowrate": 10.8
}
```

### `GET` ✅
`MTS`
Requests a piece of data from a machine. Most data is periodically reported in a `REPORT` request, but some data
requires extra work on behalf of the Monitor slave, in which an explicit request will be more efficient than repeatedly
performing the work for a scarcely used value.
```
SNIP/1 GET
Key: <Key>
```
For example:
```
SNIP/1 GET
Key: machine.power
```
`STM`
Response to a `GET` request.
```
SNIP/1 GET-RESPONSE
Key: <Key>
<Value>
```
For example:
```
SNIP/1 GET-RESPONSE
Key: machine.power
true
```

### `SET` ✅
`STM`
Sends a request to set a value of a Monitor slave.
```
SNIP/1 SET
Key: <Key>
<Value>
```
For example:
```
SNIP/1 SET
Key: machine.power
false
```
`STM`
Response to a `SET` request.
```
SNIP/1 SET-RESPONSE
[Status Headers]
```
For example:
```
SNIP/1 SET-RESPONSE
Status: Failure
Status-Code: 200
Status-Message: Machine cannot be turned off while machine.flowrate > 0.
```

---

## Information Key Values
A list of all key values that may be used for an `INFORMATION` request.

### `Machine-List` ✅
`STM` Retrieve a list of connected machines in CSV format.

### `Field-List` ✅
`STM` Retrieve a list of all fields in JSON format. If a `Target-Machine` header is set, the fields returned will only
be those originating from the specified machine.
```
SNIP/1 INFORMATION-RESPONSE
{
    "machine.power": {
        "machine": "SomeMachine",
        "mode": "both",
        "type": "boolean",
        "reported": true
    },
    "machine.power.draw": {
        "machine": "SomeMachine",
        "mode": "output",
        "type": "float",
        "reported": true
    },
    "machine.flowrate": {
        "machine": "SomeMachine",
        "mode": "output",
        "type": "float",
        "reported": false,
        "proprietary": {
            "name": "Pump Flowrate",
            "description": "Volume of water flowing through the pump.",
            "unit": "Gallons Per Hour",
            "unit_abbreviation": "GPH"
        }
    },
    "lights.overhead": {
        "machine": "LightsController",
        "mode": "both",
        "type": "boolean",
        "reported": true
    }
}
```

### `Value-List` ✅
`STM` Retrieve a list of all field values in JSON format. If a `Target-Machine` header is set, the fields returned will
only be those originating from the specified machine.
```
SNIP/1 INFORMATION-RESPONSE
{
    "machine.power": true,
    "machine.power.draw": 50.0,
    "lights.overhead": true
}
```

---