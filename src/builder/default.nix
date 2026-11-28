{
  pkgs,
  core,
}:
pkgs.stdenv.mkDerivation {
  name = "builder";
  src = ./.;
  buildInputs = [core];
}
