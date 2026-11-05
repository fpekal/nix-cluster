{
  pkgs,
  core,
}:
pkgs.stdenv.mkDerivation {
  name = "test";
  src = ./.;
  buildInputs = [core];
}
