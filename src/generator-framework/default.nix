{
  pkgs,
  core,
}:
pkgs.stdenv.mkDerivation {
  name = "generator-framework";
  src = ./.;
  buildInputs = [core pkgs.nlohmann_json];
}
