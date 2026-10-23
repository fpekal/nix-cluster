{
  pkgs,
  shared,
}:
pkgs.stdenv.mkDerivation {
  name = "test";
  src = ./.;
  buildInputs = [shared];
}
