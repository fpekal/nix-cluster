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
          test = pkgs.callPackage ./src/test/default.nix {inherit (self'.packages) core generatorFramework;};
          builder = pkgs.callPackage ./src/builder/default.nix {inherit (self'.packages) core;};
          server = pkgs.callPackage ./src/server/default.nix {inherit (self'.packages) core;};
          generatorFramework = pkgs.callPackage ./src/generator-framework/default.nix {inherit (self'.packages) core;};
          default = self'.packages.test;
          generator = self'.packages.test;
        };
      };

      flake = {
        taskTemplate = {
          taskInput,
          builderConfig,
          pkgs,
        }:
          pkgs.runCommand "example" {} ''
            sleep 17
            echo "example!" >> $out
          '';
      };
    };
}
