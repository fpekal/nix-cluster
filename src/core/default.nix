{pkgs}: let
  thirdparty = import ./thirdparty;
  amqp-cpp = pkgs.callPackage thirdparty.amqp-cpp {};
in
  pkgs.stdenv.mkDerivation {
    name = "core";
    src = ./.;
    buildInputs = [amqp-cpp pkgs.nlohmann_json];
  }
