{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs = inputs @ {flake-parts, ...}:
    flake-parts.lib.mkFlake {inherit inputs;} {
      systems = ["x86_64-linux"];

      perSystem = {
        pkgs,
        self',
        ...
      }: {
        packages = {
          core = pkgs.callPackage ./src/core/default.nix {};
          test = pkgs.callPackage ./src/test/default.nix {inherit (self'.packages) core;};
          builder = pkgs.callPackage ./src/builder/default.nix {inherit (self'.packages) core;};
          default = self'.packages.builder;
        };
      };
    };
}
