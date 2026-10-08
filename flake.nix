{
  description = "Pokémon Black 2 / White 2 decompilation tooling";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" ];
      forAll = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in {
      devShells = forAll (pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            # Keep in step with requirements.txt
            (python3.withPackages (ps: with ps; [ capstone cryptography pyelftools pyyaml ]))
            ninja
            llvm                         # llvm-objcopy, llvm-dwarfdump
            llvmPackages.clang-unwrapped # assembles the scripts for ARM; clang-format
            cargo                        # configure.py --dsd-from-source
            rustc
            gcc
          ];
        };
      });
    };
}
