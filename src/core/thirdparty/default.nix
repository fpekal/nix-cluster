{
  amqp-cpp = {pkgs}:
    pkgs.stdenv.mkDerivation (final: {
      pname = "AMQP-CPP";
      version = "4.3.27";

      src = pkgs.fetchFromGitHub {
        owner = "CopernicaMarketingSoftware";
        repo = "AMQP-CPP";
        rev = "v${final.version}";
        hash = "sha256-iaOXdDIJOBXHyjE07CvU4ApTh71lmtMCyU46AV+MGXQ=";
      };

      nativeBuildInputs = [
        pkgs.cmake
      ];

      propagatedBuildInputs = [
        pkgs.openssl.dev
      ];

      cmakeFlags = [
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
        "-DAMQP-CPP_LINUX_TCP=ON"
      ];
    });
}
