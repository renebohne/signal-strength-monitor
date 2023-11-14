// WiFi AP SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
// WiFi password
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// InfluxDB v2 server url
#define INFLUXDB_URL "https://signal-meter.digitale-dinge.de"
// InfluxDB v2 server or cloud API token (Use: InfluxDB UI -> Data -> API Tokens -> Generate API Token)
#define INFLUXDB_TOKEN "76lmf2U7h3xVbCg4_p8QKRhQ2pDUxsDJLypLccbwAuTWuRPrWueTZaQTZIlQFYBz92eOu0utZmh-inQxAmRXsg=="

// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "1b4c0edae28b9b44"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "signal-monitor"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"