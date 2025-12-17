// DeviceConfig.h
#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <Arduino.h>

// --- Hardware pins (adjust if needed) ---
static const int PIN_S0     = 19;
static const int PIN_S1     = 18;
static const int PIN_S2     = 4;
static const int PIN_S3     = 17;
static const int PIN_OUT    = 16;
static const int PIN_SERVO  = 13;
static const int PIN_BUZZER = 27;

// --- Timezone: Bolivia (UTC-4) ---
// We'll apply this offset when converting epoch -> local human times.
static const long BOLIVIA_OFFSET_SECONDS = -4L * 3600L;

// --- AWS IoT config ---
static const char THING_NAME[]       = "esp32-color-shadow";
static const char CLIENT_ID[]        = "esp32-color-shadow-proto";
static const char AWS_IOT_ENDPOINT[] = "a4ni87v7kq10b-ats.iot.us-east-2.amazonaws.com";
static const int  AWS_IOT_PORT       = 8883;

// --- MQTT command topic (immediate commands) ---
static inline String commandTopic() {
  return String("esp32/commands/") + THING_NAME;
}

// --- Certificates (PLACEHOLDERS) ---
// Replace the text below with the full PEM content including BEGIN/END lines.
// For security, consider storing on SPIFFS for production.
static const char rootCA[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

static const char deviceCert[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUd4pztSyU9zDw9CnZqia03LUmD8MwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MTEwNzE1NDUx
M1oXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALcCAbAHaQRHVrsyF240
BOuZ4TFs0M65e01j7G1iygR1M7izEVHKhgF+7up7pVgNh5rAoHyOuKNJ9H9EmddT
8FcZaFvW8Fh2857AC+FxmjViu+o3iIwrRnaquQgkGP04CFqmxOP8S6rKvbtdt4N6
DzSSxP9acNLbKOdXTCIqDkeXKmCtyLUS8ic4YTry3ilTKHOGYBWUgu0HtjaPFmx1
l/jTmJAs+4E2BI1touEapd2IlqD9785WCr4wbrtjk+b1xwaIiUKKK6QSi8J3II4s
9VXkjeRf8sB13A5BznqBHYyKIMd+oh5Hk3P0EDSNh3dQsif4Y8D3CrHD3OAWBltw
Zh8CAwEAAaNgMF4wHwYDVR0jBBgwFoAU+tMnlb4BpsxIY0pYbiLi1g+yrXkwHQYD
VR0OBBYEFCjL+pfCaFgsd4pwi7E4MoxQWD8/MAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBoV+LLrZ9kQWSCjlHzArvF+kD0
crbfkMit07nkUM6Gj58+MsK5GHFKskqjVKRpJQj3FCEEit+KPTEA00hReow/pbju
/R/YDaM5fl7B+QLa6PZP4c0fpxRZpuzfeM9sfqrlkYcSRZFjvlPDCps8l8YZHNig
YBwnkjKEL7KG1q7G6Mo0brCqRUtH2VdrJjJMdzj6043DF3yDhJ0J3Ys257tYFvfc
3EORbDP1lLtSJ6MXAYdA8xmf7szdzk45QyY/1V7EdFTGGLZAX8v2NFm9FnwUVs8Z
yQTrnCe3/bOiTc1DRsR55pcJnBDJc+DOm6+yOWmtge8jbV5JhFmf8ye++Xvr
-----END CERTIFICATE-----
)EOF";

static const char privateKey[] = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEAtwIBsAdpBEdWuzIXbjQE65nhMWzQzrl7TWPsbWLKBHUzuLMR
UcqGAX7u6nulWA2HmsCgfI64o0n0f0SZ11PwVxloW9bwWHbznsAL4XGaNWK76jeI
jCtGdqq5CCQY/TgIWqbE4/xLqsq9u123g3oPNJLE/1pw0tso51dMIioOR5cqYK3I
tRLyJzhhOvLeKVMoc4ZgFZSC7Qe2No8WbHWX+NOYkCz7gTYEjW2i4Rql3YiWoP3v
zlYKvjBuu2OT5vXHBoiJQoorpBKLwncgjiz1VeSN5F/ywHXcDkHOeoEdjIogx36i
HkeTc/QQNI2Hd1CyJ/hjwPcKscPc4BYGW3BmHwIDAQABAoIBAQCXQ3BTp/xUTgbR
GVkmfJaoifsJWDDK/aJ92B6+Vw41Ww5SFqg1G5lhuSIO6/5BZoV0Es1Txr+0L9eI
LhKeWUHpLBYG+wSTilZZG9F2GOjmQWKi+B3EBazrPrdLlFKXUe4Nx5QsAQgl9geW
y6J4aLYStVFg4scocX9An/ZMssg0wMWqAC9tZTKf8uHZY4ve5sI3gevUKPiHk0Uu
eMpqint0vE4amS4Rud4hMrDdXNEMQmrc4SENGVj3io9dUuYFjRjXMP+4ApVea+Iy
34GgKcYpGCvK3UKykDoK2nIEnVJ4rWCkrd4GOdEbnOAzIop6OKDeIUvJ4FAvW6FB
ppTEb4r5AoGBAOmf3v0TfoYsnQiz3CrMS82bS7rM+YONveL5UAp4qkXEbS9lZOes
5HjE0Q8bC1HozVQUKylnWIRMhI5/KI4gtFGYWGxA/KAWg2F+4vKExst/bjT/CH17
2kobMAoV+WtCaUkFTbqCSDplwXA//Hvp5L1Q99RPaOPRYdmqzx7Jbh1NAoGBAMiJ
Fuzze3WP/eDLYarLCojtFroRZdnZrpaaDzE3AdXAH7dQ3dxmLjmW8CG6DrYuBaB+
8EPohaMPaC1YLIHtgRA8rMeR/tQ/XMsBD/4d/PeLeCcxYI/0zj+TZ4eS/AtRKBAC
4wqXJQfxI1F/8g55ZIM6qYl88cA/0wVyZzbDVQsbAoGBAM8wKikVBctme2nBYMtP
3RYd2H500/+YT8OgSRzQQGmZNx+mc2OHECQOoD0eRd7BcH9VV6Xjcjv6REC/gq7x
UBlg22I+DAzJioCHcCuWF1tXytwTJWtr0H6SN/tp24YFIqxQmMuESRwJLBEpnfgi
yOogiXlvZ11LTtUkR4VNLGutAoGBAJBJIvemUKRL0E1XyJQMty3B+OIz9maCm328
p0Wv4GAddjR9uMQFuSiyk2CQ8FjgUCgkbVdPDChAw6IsmQl7C6vVHDQTtZidZnSh
9RHQHd02umLowiOR7nwL4SfI+BRkdkDe8uEB0yEdvV28gzsq2MkbAjTsczzyLzDy
GZVrgdsVAoGAeEZ9FQKFN+rBY0eS96BuTyeeZtr8GdI1rlD4y0DNGo/bh4Yva45D
k/YaoBHI6ZCbUd2Rtr7jbhyWmbX2NfTiYI3dL+9ctBEcz9okxjKdKB8OEdQg5YDK
wcK0duEqJT1vsXuPkfqFMQskWyjW+aMGMwbBqqlGvOr7wqYY2lEwMDs=
-----END RSA PRIVATE KEY-----
)EOF";

#endif // DEVICE_CONFIG_H
