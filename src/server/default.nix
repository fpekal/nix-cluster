{
  pkgs,
  core,
}:
pkgs.stdenv.mkDerivation {
  name = "server";
  src = ./.;
  buildInputs = [core];
}
