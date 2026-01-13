{
  description = "A high-level library for automation on Wayland";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
    in
    {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.stdenv.mkDerivation {
            pname = "waymo";
            version = "0.1.0";
            src = ./.;

            nativeBuildInputs = with pkgs; [
              pkg-config
              cmake
              wayland-scanner
              ninja
            ];

            buildInputs = with pkgs; [
              wayland
              libxkbcommon
              libffi
            ];

            cmakeFlags = [
              "-DDO_INSTALL=ON"
              "-DBUILD_SHARED=ON"
              "-DBUILD_STATIC=ON"
              "-DGENERATE_PROTOCOLS=ON"
              "-DUSE_CCACHE=OFF"
	      "-DPKG_CONFIG=ON"
            ];
          };
        });

      devShells = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.mkShell {
            inputsFrom = [ self.packages.${system}.default ];

            packages = with pkgs; [
              gdb
              clang-tools
            ];
          };
        });
    };
}
