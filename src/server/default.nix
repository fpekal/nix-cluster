{
  pkgs,
  core,
}:
pkgs.stdenv.mkDerivation {
  name = "server";
  src = ./.;
  buildInputs = [core pkgs.nlohmann_json];
}
