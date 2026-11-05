{pkgs}:
pkgs.stdenv.mkDerivation {
  name = "core";
  src = ./.;
  buildInputs = [pkgs.nlohmann_json];
}
