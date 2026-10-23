{pkgs}:
pkgs.stdenv.mkDerivation {
  name = "shared";
  src = ./.;
  buildInputs = [pkgs.nlohmann_json];
}
