
set(EMBED_FILES "")
list(APPEND EMBED_FILES
        "certs/ws_ca_cert.pem"
        "certs/chat_ca_cert.pem"
        )
idf_component_register(
    SRC_DIRS 
        "."
        "app"
    INCLUDE_DIRS 
        "."
        "app"
    EMBED_TXTFILES "${EMBED_FILES}")

target_compile_options(${COMPONENT_LIB} PUBLIC "-Wno-format-truncation")