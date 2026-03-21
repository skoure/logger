from conan import ConanFile

class LoggerConan(ConanFile):
    name = "logger"
    version = "1.0"
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeToolchain", "CMakeDeps"
    options = {"with_log4cxx": [True, False], "with_spdlog": [True, False], "with_cpptrace": [True, False]}
    default_options = {"with_log4cxx": False, "with_spdlog": False, "with_cpptrace": True}

    def requirements(self):
        self.requires("gtest/1.16.0")

        if self.options.with_log4cxx:
            self.requires("log4cxx/1.5.0")

        if self.options.with_spdlog:
            self.requires("spdlog/1.16.0")

        if self.options.with_cpptrace:
            self.requires("cpptrace/0.7.5")

    def configure(self):
        self.options["gtest"].shared = False
        self.options["gtest"].build_gmock = True

        self.options["log4cxx"].shared = False
