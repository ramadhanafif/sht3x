idf_component_register(SRCS "sht3x-example.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver
                    PRIV_REQUIRES sht3x
)
