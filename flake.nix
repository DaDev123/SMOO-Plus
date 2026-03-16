{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    self.submodules = true;
  };
  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }:
    flake-utils.lib.eachDefaultSystem
    (
      system: let
        pkgs = import nixpkgs {
          inherit system;
        };
        build = import ./sys/nix pkgs;
      in
        with pkgs; rec {
          formatter = pkgs.alejandra;
          devShells.default = mkShell.override { stdenv = llvmPackages.libcxxStdenv; } {
            buildInputs = build.dependencies ++ [inetutils];
            SWITCHTOOLS = "${build.localPackages.switch-tools}/bin";
          };
        }
    );
}
