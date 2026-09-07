library ieee;
use ieee.std_logic_1164.all;

entity tlm_p3_gate_tb is
end entity;

architecture test of tlm_p3_gate_tb is

    signal a0 : std_logic_vector(7 downto 0);
    signal a1 : std_logic_vector(7 downto 0);
    signal a2 : std_logic_vector(7 downto 0);
    signal a3 : std_logic_vector(7 downto 0);

    signal y0 : std_logic_vector(7 downto 0);
    signal y1 : std_logic_vector(7 downto 0);
    signal y2 : std_logic_vector(7 downto 0);
    signal y3 : std_logic_vector(7 downto 0);

    signal test_passed : boolean := true;

    procedure check_vector(
        constant test_name : in string;
        constant exp_y0, exp_y1, exp_y2, exp_y3 : in std_logic_vector(7 downto 0);
        signal act_y0, act_y1, act_y2, act_y3 : in std_logic_vector(7 downto 0);
        signal test_passed : inout boolean
    ) is
        variable mismatch : boolean := false;
    begin
        if act_y0 /= exp_y0 then
            report test_name & ": y0 mismatch" severity error;
            mismatch := true;
        end if;
        if act_y1 /= exp_y1 then
            report test_name & ": y1 mismatch" severity error;
            mismatch := true;
        end if;
        if act_y2 /= exp_y2 then
            report test_name & ": y2 mismatch" severity error;
            mismatch := true;
        end if;
        if act_y3 /= exp_y3 then
            report test_name & ": y3 mismatch" severity error;
            mismatch := true;
        end if;
        if mismatch then
            test_passed := false;
        else
            report test_name & ": PASSED" severity note;
        end if;
    end procedure;

begin

    DUT : entity work.tlm_p3_gate(structural)
        port map (
            a0 => a0, a1 => a1, a2 => a2, a3 => a3,
            y0 => y0, y1 => y1, y2 => y2, y3 => y3
        );

    process
    begin

        -- Test 1: Mandatory AES vector D4 BF 5D 30 -> 04 66 81 E5
        a0 <= x"D4"; a1 <= x"BF"; a2 <= x"5D"; a3 <= x"30";
        wait for 1 ns;
        check_vector("Test1_D4_BF_5D_30", x"04", x"66", x"81", x"E5",
                      y0, y1, y2, y3, test_passed);

        -- Test 2: Zero vector
        a0 <= x"00"; a1 <= x"00"; a2 <= x"00"; a3 <= x"00";
        wait for 1 ns;
        check_vector("Test2_Zero", x"00", x"00", x"00", x"00",
                      y0, y1, y2, y3, test_passed);

        -- Test 3: b7 = 0 (no reduction)
        a0 <= x"01"; a1 <= x"02"; a2 <= x"04"; a3 <= x"08";
        wait for 1 ns;
        check_vector("Test3_b7_0", x"08", x"01", x"13", x"15",
                      y0, y1, y2, y3, test_passed);

        -- Test 4: b7 = 1 and reduction by 0x1B
        a0 <= x"FF"; a1 <= x"00"; a2 <= x"00"; a3 <= x"00";
        wait for 1 ns;
        check_vector("Test4_FF_00_00_00", x"E5", x"FF", x"FF", x"1A",
                      y0, y1, y2, y3, test_passed);

        -- Test 5: Repeated bytes (uniform column preserved)
        a0 <= x"50"; a1 <= x"50"; a2 <= x"50"; a3 <= x"50";
        wait for 1 ns;
        check_vector("Test5_Repeated", x"50", x"50", x"50", x"50",
                      y0, y1, y2, y3, test_passed);

        -- Test 6: Alternating high-bit values
        a0 <= x"80"; a1 <= x"00"; a2 <= x"80"; a3 <= x"00";
        wait for 1 ns;
        check_vector("Test6_Alt_80", x"F0", x"A0", x"F0", x"A0",
                      y0, y1, y2, y3, test_passed);

        -- Test 7: All XOR paths (alternating pattern)
        a0 <= x"AA"; a1 <= x"55"; a2 <= x"AA"; a3 <= x"55";
        wait for 1 ns;
        check_vector("Test6_AA_55", x"FF", x"00", x"FF", x"00",
                      y0, y1, y2, y3, test_passed);

        -- Test 8: Boundary byte values
        a0 <= x"FF"; a1 <= x"FF"; a2 <= x"FF"; a3 <= x"FF";
        wait for 1 ns;
        check_vector("Test7_FF_FF", x"00", x"00", x"00", x"00",
                      y0, y1, y2, y3, test_passed);

        -- Test 9: Walking 1 pattern
        a0 <= x"01"; a1 <= x"00"; a2 <= x"00"; a3 <= x"00";
        wait for 1 ns;
        check_vector("Test9_01_00", x"01", x"01", x"00", x"01",
                      y0, y1, y2, y3, test_passed);

        -- Test 10: Walking 1 in MSB position (triggers reduction)
        a0 <= x"80"; a1 <= x"00"; a2 <= x"00"; a3 <= x"00";
        wait for 1 ns;
        check_vector("Test10_80_00", x"9B", x"1B", x"00", x"9B",
                      y0, y1, y2, y3, test_passed);

        report "=== Testbench Complete ===" severity note;
        if test_passed then
            report "ALL TESTS PASSED" severity note;
        else
            report "SOME TESTS FAILED" severity error;
        end if;
        wait;

    end process;

end architecture;
