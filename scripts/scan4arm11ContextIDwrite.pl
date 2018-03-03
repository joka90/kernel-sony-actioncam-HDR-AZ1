#!/usr/bin/perl -w

# Copyright ARM Limited, 2006. All rights reserved.
# Designed for operation with gcc
# $Revision: 1.6 $
# $Date: 2006/07/11 17:09:45 $

use strict;
use File::Glob (':glob', ':nocase');
use Text::Tabs;
use Getopt::Long;

$| = 1;

my $file_ext = ".o";

my $help = 0;
GetOptions (
            "h"   =>  \$help,
           );

if ($help or scalar @ARGV == 0) {
    usage_exit();
}


my $file;
my @files;
my $issues = 0;

while ($file = shift @ARGV) {
    if ($file =~ m/[*?]/) {
        # File pattern argument so expand it...
        @files = bsd_glob("$file");
    } else {
        @files = ($file);
    }

    foreach $file(@files) {
        $issues += scan_file($file);
    }
}

print "\nTotal number of issues found: $issues \n";

if ($issues != 0) {
    print "\nInput files might be affected by errata 408022 (ARM1136) or 415047 (ARM1176).";
    print "\nPlease contact ARM support (support-cores@"."arm.com) for further analysis.\n";
    exit(1);
}

exit(0);


sub usage_exit {
    print "ARM scanning script for errata 408022 (ARM1136) or 415047 (ARM1176).\n";
    print "Version 1.0\n\n";
    print "Usage: $0 file, file, file ...\n";
    print "(where each file is a disassembled file -- use arm-linux-objdump to create)\n";
    print "Not guaranteed to cover all possible cases\n";
    print "Options: \n";
    print "   -h : this help \n";
    exit(1);
}

sub scan_file {
    my $instr0 = "";
    my $instr1 = "";
    my $instrmn = "";
    my $instrmn1 = "";
    my $instrmn2 = "";
    my ($file) = @_;
    my ($line, $prev_instr) = ("", "");
    my $t;
    my $r;
    my $i;
    my $hexaddress;
    my $targethex;
    my @branchtargets;
    my @ContextIDWriteInsnAddrs;
    my $ContextIDWriteInstr = 0;
    my $InstrnBx_r14 = 0;
    my $InstrMov_pc_r14 = 0;
    my $n_ContextIDWriteInsnAddrs;
    my $n_branchtargets;
    my $index;
    my $issue_string;
    my $issue_count = 0;
    my $index_bt;
    my $line_count = 0;

    open IN, "$file" or die "Could not open file $file";

    print "Scanning $file ... ";

    while ($line = <IN>) {
        $ContextIDWriteInstr = 0;
        $InstrnBx_r14 = 0;
        $InstrMov_pc_r14 = 0;
        $issue_string = '';

        chomp $line;
        $line = lc $line;
        $line = "   ".$line;
        my ($pad, $address, $hex,  $instr, $args, $comment) = split /\s\s+/, expand($line), 6;


        # Skip lines not containing ARM code...
        if (!defined $address || !defined $hex || length($hex) != 8 || substr($address, -1) ne ":"   || hex(substr($address, 0, -1)) % 4 != 0) {
            $prev_instr = "";
            next;
        }

        # Count number of lines scanned for reporting at end
        ++$line_count;

        #Just store the mnemonics of the instructions for the moment
        $instrmn2 = $instrmn1;
        $instrmn1 = $instrmn;
        $instrmn = $instr;

        # useful to have the address as a hex value:
        $hexaddress = hex(substr($address, 0, -1));

 	# sort out the argument list
        if (defined $args && substr($args, 0, 1) ne ";") {
            $instr = $instr ." " . $args;
        }

        # set up a shift register of instructions
        $instr1 = $instr0;
        $instr0 = $instr;


        #####################################################
        # There are THREE distinct cases to be taken care of.
        #####################################################

        # CASE 1:
        # a) An instruction that can cause a branch that is not a load, store or RFE.
        # b) An MCR which updates the Context ID register.
        # So, the algorithm is
        #    1. Find out if instr2 falls in one of the PC modifying class of instrs.
        #    2. Find out if instr1 is ContextID Write (MCR p15, 0, <Rd>, c13, c0, 1).
        #
        # This case is described in the erratum description as the first
        # half of condition 1, and the whole of condition 2.


        # CASE 2:
        # a) There must be a conditional immediate branch instruction.
        # b) an MCR which updates the Context ID register must be at the branch
        #    target address.
        # So, the algorithm is
        #    1. Find out if conditional immediate branch goes to ContextID Write instr.
	#       Remember the fact that the branch could be forward or backward.
        #    2. Find out if instr1 is ContextID Write (MCR p15, 0, <Rd>, c13, c0, 1).
        #
        # This cases described in the erratum description as the second
        # half of condition 1.


        # CASE 3:
	# a) The following two instruction classes must appear in sequential order
	#    1. A saturating, multiplication or MRS instruction
        #    2. A BX R14 or MOV PC, R14 instruction
        # b) The saturating, multiplication or MRS instruction must have R14 as
        #    the destination register.
        #
        # In extreme circumstances the branch could be predicted by the
        # return stack, and the return stack could point to an ASID
        # write. We cannot check where the return stack points, but the two
        # conditions above already highly implausible.
        #
        # This case is not described in the erratum description because it
        # is not a practical case.  There is no sensible code sequence
        # which would use a saturating, multiplication or MRS instruction
        # to calculate an address to branch to.  However, for complete
        # safety, the script searches for this case anyway.  Please contact
        # ARM support if you encounter this case.


        # Find out if you match a Context ID write instruction FOR CASE1
        if($instr =~ m/^mcr\w* *p?15, *(0x)?0, *(r\d+|sl|fp|ip|sp|lr|pc), *cr?13, *cr?0, *\{?1\}?/) {
           $ContextIDWriteInstr = 1;
           # store the hex address of this instruction in a list.
	   #print "match found for Context ID write MCR \n";
           #print "$instr \n";
           $hexaddress = hex(substr($address,0,-1));
           push @ContextIDWriteInsnAddrs, $hexaddress;

        }


        # 1. If it is a conditional immediate branch, store the target address of the branch in a list.
        # 2. take each element in the contextidaddresses list and see if any of them matches
        #    with those in the conditional-branch-target-address list. For CASE2


        if($instr =~ m/^(b|bl)(eq|ne|cs|hs|cc|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le)/) {
            ($i,$t,$r) = split /\s+/, $instr;
            next unless defined $t;
            $t = "0x".$t;
            $targethex = hex($t);
            push @branchtargets, $targethex;
            #print "The branch is to $i $t; The instrn was $instr \n"
        }


        # Find out if the instrn is BX R14 ; FOR CASE3
        if($instr =~ m/bx/){
	   ($i, $t, $r) = split /\s+/, $instr;
           if(($t =~ "r14") || ($t =~ "lr")){
              $InstrnBx_r14 = 1;

              #print "BX R14 hit \n";
           }
        }

        # Find out if the instruction is MOV PC, R14 ; FOR CASE3
        if($instr =~ m/mov/){
           ($i, $t, $r) = split /\s+/, $instr;
           if( (($t =~ "r15") || ($t =~ "pc") ) && (($r =~ "r14") || ($r =~ "lr")) ){
	      $InstrMov_pc_r14 = 1;
              #print "MOV PC, R14 \n";
           }
        }

        # CASE1
        if($ContextIDWriteInstr ) {

            if($instrmn1 =~ m/^((b|bl)(eq|ne|cs|hs|cc|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le))/){
                # Check if it's a conditional branch instruction.
                $issue_string = "Conditional branch followed by ASID write";
            }
            elsif($instrmn1 =~ m/^(swi)/){
                # Check if it's an SVC instruction.
                $issue_string = "SWI/SVC followed by ASID write";
            }
            elsif($instrmn1 =~ m/^(adc|add|and|bic|eor|orr|rsb|rsc|sbc|sub)/){
                # For data processing instructions check whether their
                # destination register is R15.
                ($i, $t, $r) = split /\s+/, $instr1;
                if(($t =~ "r15") || ($t =~ "pc")){
                    $issue_string = "Data-processing instruction that writes to the PC followed by ASID write";
                }
            }
            elsif($instrmn1 =~ m/^(bx|blx|bxj)/ and
                  ($instrmn2 =~ m/^(adc|add|and|bic|eor|orr|rsb|rsc|sbc|sub|mov|mvn|mla|mul)(eq|ne|cs|hs|cc|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le)?s/ or
                   $instrmn2 =~ m/^(cmp|cmn|msr|teq|tst)/)) {
                # Check if it's a BX variant instruction preceded by a flag-setting instruction.
                $issue_string = "Flag-setting data-processing instruction followed by BX followed by ASID write";
            }
            elsif($instrmn1 =~ m/^(mov|mvn)/ and
                  ($instrmn2 =~ m/^(adc|add|and|bic|eor|orr|rsb|rsc|sbc|sub|mov|mvn|mla|mul)(eq|ne|cs|hs|cc|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le)?s/ or
                   $instrmn2 =~ m/^(cmp|cmn|msr|teq|tst)/)) {
                # Check if it's a MOV/MVN instruction preceded by a flag-setting instruction.
                ($i, $t, $r) = split /\s+/, $instr1;
                if(($t =~ "r15") || ($t =~ "pc")){
                    $issue_string = "Flag-setting data-processing instruction followed by MOV/MVN to the PC followed by ASID write";
                }
            }
         } # END of "if(($ContextIDWriteInstr)

         # CASE 3
         if(($InstrnBx_r14) || ($InstrMov_pc_r14)) {
             my $issue3hit = 0;
             # Check if the last instruction was a multiply, mrs or saturating instrn with R14 as its dest reg
             if($instrmn1 =~ m/^(usat|ssat|usat16|ssat16|qadd|qsub|qdadd|qdsub|qadd8|qadd16|qsub8|qsub16|shadd8|shadd16|shsub8|shsub16|uqadd8|uqadd16|uqsub8|uqsub16|uhadd8|uhadd16|uhsub8|uhsub16|qaddsubx|qsubaddx|shaddsubx|shsubaddx|uqaddsubx|uhaddsubx|uhsubaddx)/){
                 ($i, $t, $r) = split /\s+/, $instr1;
                 if(($t =~ "r14") || ($t =~ "lr")){
                     $issue3hit = 1;
                 }
             }
             elsif ($instrmn1 =~ m/^(mla|mrs|mul|smlabb|smlabt|smlatb|smlatt|smlad|smlawb|smlawt|smlsd|smmla|smmls|smmlsr|smmul|smmulr|smuad|smuadx|smulbb|smulbt|smultb|smultt|smulwb|smulwt|smusd|smusdx)/){
                 #Checking for a multiply instruction or mrs with R14 as register, mult insns with only one dest reg
                 # print "instruction mnemonic is $instrmn1 \n";
                 ($i, $t, $r) = split /\s+/, $instr1;
                 if(($t =~ "r14") || ($t =~ "lr")){
                     $issue3hit = 1;
                 }
             }
             elsif ($instrmn1 =~ m/^(smlal|smlalbb|smlalbt|smlaltb|smlaltt|smlald|smlsld|smlsldx|smull|umaal|umlal|umull)/) {
                 #Checking for mul instructions which have two destination registers and r14 as one of their dest regs
                 ($i, $t, $r) = split /\s+/, $instr1;
                 if(($t =~ "r14") || ($t =~ "lr") ||($r =~ "r14") || ($r =~ "lr")){
                     $issue3hit = 1;
                 }
             }

             if ($issue3hit) {
                 $issue_string = "Saturating instruction, multiply or MRS to r14 followed by return stack predictable branch.\n".
                     "  - Please contact ARM support, because this case is not expected to be encountered";
             }
         } #End of ($InstrnBx_r14) || ($InstrMov_pc_r14))"

        if ($issue_string) {
            if ($issue_count == 0) {
                print "FAILED\n";
            }
            print "Address $address $issue_string\n";
            ++$issue_count;
        }
    }
    close IN;
    #    Take the elements in @branchtargets and see if it matches to any of those in $ContextIDWriteInsnAddrs

    # CASE2
    $n_ContextIDWriteInsnAddrs = @ContextIDWriteInsnAddrs;
    $n_branchtargets = @branchtargets;

    #print "mcrs are $n_ContextIDWriteInsnAddrs and condition branches are $n_branchtargets\n";
    for($index = 0; $index < $n_ContextIDWriteInsnAddrs ; $index++){
        for($index_bt = 0; $index_bt < $n_branchtargets; $index_bt++) {
            if($ContextIDWriteInsnAddrs[$index] == $branchtargets[$index_bt]) {
                if ($issue_count == 0) {
                    print "FAILED\n";
                }
                printf "Address %x: Conditional branch to ASID write\n", $ContextIDWriteInsnAddrs[$index];
                ++$issue_count;
            }
        }
    }

    if ($issue_count == 0) {
#        if ($line_count == 0) {
#            print "FAILED -- no lines scanned\n";
#            die "Files must be in disassembled format -- use arm-linux-objdump to create\n";
#       }
#        else {
#            print "passed ($line_count lines scanned)\n";
#        }
    }
    return $issue_count;
}

