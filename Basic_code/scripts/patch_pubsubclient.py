from pathlib import Path

Import("env")

if env.subst("$PIOENV") == "esp8266":
    source = (
        Path(env.subst("$PROJECT_LIBDEPS_DIR"))
        / env.subst("$PIOENV")
        / "PubSubClient"
        / "src"
        / "PubSubClient.cpp"
    )

    if source.exists():
        contents = source.read_text(encoding="utf-8")
        malformed_check = """        if (this->bufferSize < MQTT_MAX_HEADER_SIZE + 2+strnlen(topic, this->bufferSize) + plength) {
    unsigned int rc = 0;
    unsigned int expectedLength;
        }
"""
        correct_check = """        if (this->bufferSize < MQTT_MAX_HEADER_SIZE + 2+strnlen(topic, this->bufferSize) + plength) {
            // Too long
            return false;
        }
"""

        contents = contents.replace(malformed_check, correct_check)
        contents = contents.replace(
            "    int expectedLength;\n\n    if (!connected()) {",
            "    unsigned int expectedLength;\n\n    if (!connected()) {",
        )
        source.write_text(contents, encoding="utf-8")
