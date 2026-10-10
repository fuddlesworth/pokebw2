#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    VMCall L_0024
    VMCall L_0324
    FieldSetTeleportZone 18
    MoneyAdd 3000
    VMHalt

L_0024:
    FlagSet 989
    FlagSet 607
    FlagSet 613
    FlagSet 604
    FlagSet 605
    FlagSet 615
    FlagSet 767
    FlagSet 768
    FlagSet 769
    FlagSet 621
    FlagSet 614
    FlagSet 660
    FlagSet 667
    FlagSet 645
    WorkSetConst 0x4165, 1
    FlagSet 740
    FlagSet 739
    FlagSet 965
    FlagSet 702
    FlagSet 770
    FlagSet 771
    FlagSet 1014
    FlagSet 761
    FlagSet 762
    FlagSet 733
    FlagSet 734
    FlagSet 735
    FlagSet 736
    FlagSet 737
    FlagSet 727
    FlagSet 743
    FlagSet 741
    FlagSet 2430
    FlagSet 730
    FlagSet 996
    FlagSet 724
    FlagSet 723
    FlagSet 722
    FlagSet 721
    FlagSet 746
    FlagSet 751
    FlagSet 1030
    FlagSet 753
    FlagSet 754
    FlagSet 756
    FlagSet 758
    FlagSet 717
    FlagSet 716
    FlagSet 697
    FlagSet 712
    FlagSet 710
    FlagSet 894
    FlagSet 891
    FlagSet 890
    FlagSet 772
    FlagSet 773
    FlagSet 774
    FlagSet 775
    FlagSet 782
    FlagSet 784
    FlagSet 785
    FlagSet 794
    FlagSet 795
    FlagSet 796
    FlagSet 797
    FlagSet 798
    FlagSet 800
    FlagSet 1034
    FlagSet 1010
    FlagSet 792
    FlagSet 805
    FlagSet 808
    FlagSet 985
    FlagSet 1008
    FlagSet 811
    FlagSet 810
    FlagSet 814
    FlagSet 821
    FlagSet 822
    FlagSet 823
    FlagSet 824
    FlagSet 825
    FlagSet 826
    FlagSet 828
    FlagSet 853
    FlagSet 832
    FlagSet 833
    FlagSet 834
    FlagSet 839
    FlagSet 848
    FlagSet 854
    FlagSet 842
    FlagSet 843
    FlagSet 977
    FlagSet 1029
    FlagSet 2560
    FlagSet 855
    FlagSet 857
    FlagSet 856
    FlagSet 859
    FlagSet 860
    FlagSet 861
    FlagSet 862
    FlagSet 863
    FlagSet 864
    FlagSet 865
    FlagSet 866
    FlagSet 895
    FlagSet 896
    FlagSet 878
    FlagSet 880
    FlagSet 886
    FlagSet 888
    FlagSet 889
    FlagSet 968
    FlagSet 969
    FlagSet 1031
    FlagSet 882
    FlagSet 1004
    FlagSet 1003
    FlagSet 1011
    FlagSet 900
    FlagSet 898
    FlagSet 897
    FlagSet 917
    FlagSet 916
    FlagSet 918
    FlagSet 919
    FlagSet 920
    FlagSet 912
    FlagSet 941
    FlagSet 913
    FlagSet 1035
    FlagSet 925
    FlagSet 926
    FlagSet 927
    FlagSet 695
    FlagSet 944
    FlagSet 942
    FlagSet 945
    FlagSet 960
    FlagSet 950
    FlagSet 951
    FlagSet 953
    FlagSet 955
    FlagSet 957
    FlagSet 683
    FlagSet 970
    FlagSet 971
    FlagSet 972
    FlagSet 973
    FlagSet 974
    FlagSet 978
    FlagSet 979
    FlagSet 986
    FlagSet 627
    FlagSet 628
    FlagSet 629
    FlagSet 630
    FlagSet 631
    FlagSet 991
    FlagSet 992
    FlagSet 994
    FlagSet 995
    FlagSet 691
    FlagSet 692
    FlagSet 693
    FlagSet 694
    FlagSet 1007
    FlagSet 1016
    FlagSet 1017
    FlagSet 1018
    FlagSet 1019
    FlagSet 1020
    FlagSet 1021
    FlagSet 1022
    FlagSet 1023
    FlagSet 1028
    FlagSet 1032
    FlagSet 1033
    FlagSet 1037
    FlagSet 1041
    FlagSet 1042
    FlagSet 1043
    FlagSet 1044
    FlagSet 1045
    FlagSet 1046
    FlagSet 1047
    FlagSet 1048
    FlagSet 1049
    FlagSet 1050
    VMReturn

L_0324:
    GameGetVersion 0x8010
    WorkCmpConst 0x8010, 23
    VMJumpIf CMP_EQ, L_033B
    VMJump L_0347

L_033B:
    Cmd_0209 0, 1
    VMJump L_0366

L_0347:
    WorkCmpConst 0x8010, 22
    VMJumpIf CMP_EQ, L_035A
    VMJump L_0366

L_035A:
    Cmd_0209 0, 2
    VMJump L_0366

L_0366:
    TrainerCardGetSex 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_037D
    VMJump L_0389

L_037D:
    Cmd_0209 1, 1
    VMJump L_03A8

L_0389:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_039C
    VMJump L_03A8

L_039C:
    Cmd_0209 1, 2
    VMJump L_03A8

L_03A8:
    Cmd_0209 29, 1
    VMReturn

Script_3:
    VMCall L_0024
    FieldSetTeleportZone 3
    FlagSet 106
    FlagSet 2401
    FlagSet 2402
    GiveRunningShoes
    VMHalt

Script_2:
    VMStackPushFlag 2400
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03E3
    VMCall L_06DD

L_03E3:
    PokePartyRecoverAll
    FieldSetTeleportZone 18
    FieldSetNextZone 428, 2, 13, 0, 5
    FlagSet EVENT_FLAG_CONTINUE_SCRIPT
    VMStackPushFlag 679
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 257
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0420
    FlagReset 679

L_0420:
    VMStackPushFlag 801
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 301
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0447
    FlagReset 801

L_0447:
    VMStackPushFlag 802
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 303
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_046E
    FlagReset 802

L_046E:
    VMStackPushFlag 810
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 330
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0495
    FlagReset 810

L_0495:
    VMStackPushFlag 374
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04B2
    WorkSetConst 0x410c, 0
    FlagSet 1037

L_04B2:
    VMStackPushFlag 380
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x4074
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04DB
    WorkSetConst 0x4074, 1

L_04DB:
    VMStackPushFlag 379
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x4073
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0504
    WorkSetConst 0x4073, 1

L_0504:
    VMStackPushFlag 396
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x4116
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_052D
    WorkSetConst 0x4116, 1

L_052D:
    VMStackPushFlag 397
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x4117
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0556
    WorkSetConst 0x4117, 1

L_0556:
    VMStackPushFlag 398
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x4118
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_057F
    WorkSetConst 0x4118, 1

L_057F:
    VMStackPushFlag 928
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 403
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05A6
    FlagReset 928

L_05A6:
    VMStackPushFlag 921
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 399
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05CD
    FlagReset 921

L_05CD:
    VMStackPushFlag 922
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 400
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05F4
    FlagReset 922

L_05F4:
    VMStackPushFlag 923
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 401
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_061B
    FlagReset 923

L_061B:
    VMStackPushFlag 924
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 402
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0642
    FlagReset 924

L_0642:
    VMStackPushFlag 946
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 420
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0669
    FlagReset 946

L_0669:
    VMStackPushFlag 667
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 234
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x411b
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_06A0
    FlagReset 667

L_06A0:
    VMStackPushFlag 1004
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 249
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag 480
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_06DB
    FlagReset 1004
    FlagSet 1003

L_06DB:
    VMHalt

L_06DD:
    FlagSet 2400
    HollowRivalCmd_0262 0, 0
    HollowRivalCmd_0262 1, 40
    HollowRivalCmd_0262 2, 14
    HollowRivalCmd_0262 3, 0
    HollowRivalCmd_0262 4, 0
    FlagSet 718
    WorkSetConst 0x40c4, 1
    FlagReset 444
    FlagReset 2546
    FlagReset 993
    FlagReset 800
    FlagReset 1009
    FlagSet 794
    FlagSet 1010
    MapReplaceSetEvent 0, 0, 0
    FlagReset 645
    FlagReset 749
    FlagSet 809
    WorkSetConst 0x40e1, 1
    FlagSet 976
    FlagReset 977
    FlagSet 975
    MapReplaceSetEvent 1, 1, 1
    WorkSetConst 0x414a, 1
    WorkSetConst 0x414b, 1
    WorkSetConst 0x414c, 1
    WorkSetConst 0x414d, 1
    WorkSetConst 0x414e, 1
    WorkSetConst 0x4149, 2
    FlagSet 883
    Cmd_00E4 1
    FlagSet 904
    FlagReset 917
    WorkSetConst 0x411c, 1
    FlagReset 945
    WorkSetConst 0x40a0, 2
    WorkSetConst 0x4098, 1
    FlagSet 686
    WorkSetConst 0x4123, 1
    FlagReset 991
    FlagSet 644
    FlagReset 1008
    FlagSet 1033
    VMReturn
    VMReturn
    .balign 4, 0
