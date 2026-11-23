{
  pkgs,
  core,
  generatorFramework,
}:
pkgs.stdenv.mkDerivation {
  name = "test";
  src = ./.;
  buildInputs = [core generatorFramework];
}
