{
  description = "GalleryWM-oriented fork of neuswc";

  inputs = 
  {
    nixpkgs.url = "nixpkgs/nixos-unstable";

    neuwld-gwm =
    {
      url = "github:altaeder/neuwld-gwm";
    };
  };

  outputs = 
  { self, nixpkgs, neuwld-gwm, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs
      {
        inherit system;
      };
    in
    {
      packages.${system}.default = 
      pkgs.stdenv.mkDerivation
      {
        pname = "neuswc-gwm";
        version = "0.0";

        src = self;

        nativeBuildInputs = with pkgs;
        [
          meson
          ninja
          pkg-config
          wayland-scanner
        ];

        buildInputs = with pkgs;
        [
          # GalleryWM depedency
          neuwld-gwm.packages.${system}.default

          # Core
          libinput
          udev
          fontconfig

          # Wayland
          wayland
          wayland-protocols
          pixman
          libxkbcommon

          # DRM // Graphics
          libdrm
          xf86-video-amdgpu
          libGL
          libGLU
          mesa
          mesa-gl-headers
          mesa_glu
          libgbm
          libglvnd
          egl-gbm
          egl-wayland

          # Xwayland
          xwayland
          libxcb
          libxcb-wm
          xcbutil
          xcbutilwm
          xcbutilkeysyms
          xcbutilrenderutil
          xcbutilimage
        ];

        mesonFlags =
        [
          "-Dvideo=drm"
          "-Dinput=libinput"
          "-Dxwayland=enabled"
          "-Dudev=enabled"
          "-Dextra=true"
          "-Dexample=false"
        ];
      };

      devShells.${system}.default = pkgs.mkShell 
      {
        packages = with pkgs;
        [
          meson
          ninja
          pkg-config
          wayland-scanner

          gcc
          fontconfig

          # GWM depdency
          neuwld-gwm.packages.${system}.default

          # Dev dependencies
          wayland
          wayland-protocols
          pixman
          libxkbcommon
          libinput
          udev
          libdrm

          # Graphics
          xf86-video-amdgpu
          libGL
          libGLU
          mesa
          mesa-gl-headers
          mesa_glu
          libgbm
          libglvnd
          egl-gbm
          egl-wayland

          # Xwayland
          xwayland
          libxcb
          libxcb-wm
          xcbutil
          xcbutilwm
          xcbutilkeysyms
          xcbutilrenderutil
          xcbutilimage
        ];
      };
    };
}