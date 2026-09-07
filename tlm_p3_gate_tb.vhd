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

begin

    DUT : entity work.tlm_p3_gate(structural)
        port map (
            a0 => a0,
            a1 => a1,
            a2 => a2,
            a3 => a3,
            y0 => y0,
            y1 => y1,
            y2 => y2,
            y3 => y3
        );

    process
    begin

        ----------------------------------------------------------------
        -- Mandatory AES vector
        -- D4 BF 5D 30 -> 04 66 81 E5
        ----------------------------------------------------------------

        a0 <= x"D4";
        a1 <= x"BF";
        a2 <= x"5D";
        a3 <= x"30";

        wait for 1 ns;

        assert y0 = x"04"
            report "D4 BF 5D 30: y0 failure"
            severity error;

        assert y1 = x"66"
            report "D4 BF 5D 30: y1 failure"
            severity error;

        assert y2 = x"81"
            report "D4 BF 5D 30: y2 failure"
            severity error;

        assert y3 = x"E5"
            report "D4 BF 5D 30: y3 failure"
            severity error;


        ----------------------------------------------------------------
        -- Zero vector
        ----------------------------------------------------------------

        a0 <= x"00";
        a1 <= x"00";
        a2 <= x"00";
        a3 <= x"00";

        wait for 1 ns;

        assert y0 = x"00"
            report "Zero vector: y0 failure"
            severity error;

        assert y1 = x"00"
            report "Zero vector: y1 failure"
            severity error;

        assert y2 = x"00"
            report "Zero vector: y2 failure"
            severity error;

        assert y3 = x"00"
            report "Zero vector: y3 failure"
            severity error;


        ----------------------------------------------------------------
        -- b7 = 0
        ----------------------------------------------------------------

        a0 <= x"01";
        a1 <= x"02";
        a2 <= x"04";
        a3 <= x"08";

        wait for 1 ns;

        assert y0 = x"08"
            report "01 02 04 08: y0 failure"
            severity error;

        assert y1 = x"01"
            report "01 02 04 08: y1 failure"
            severity error;

        assert y2 = x"13"
            report "01 02 04 08: y2 failure"
            severity error;

        assert y3 = x"15"
            report "01 02 04 08: y3 failure"
            severity error;


        ----------------------------------------------------------------
        -- b7 = 1 and reduction by 0x1B
        ----------------------------------------------------------------

        a0 <= x"FF";
        a1 <= x"00";
        a2 <= x"00";
        a3 <= x"00";

        wait for 1 ns;

        assert y0 = x"E5"
            report "FF 00 00 00: y0 failure"
            severity error;

        assert y1 = x"FF"
            report "FF 00 00 00: y1 failure"
            severity error;

        assert y2 = x"FF"
            report "FF 00 00 00: y2 failure"
            severity error;

        assert y3 = x"1A"
            report "FF 00 00 00: y3 failure"
            severity error;


        ----------------------------------------------------------------
        -- Repeated bytes
        -- MixColumns must preserve a uniform column.
        ----------------------------------------------------------------

        a0 <= x"50";
        a1 <= x"50";
        a2 <= x"50";
        a3 <= x"50";

        wait for 1 ns;

        assert y0 = x"50"
            report "Repeated bytes: y0 failure"
            severity error;

        assert y1 = x"50"
            report "Repeated bytes: y1 failure"
            severity error;

        assert y2 = x"50"
            report "Repeated bytes: y2 failure"
            severity error;

        assert y3 = x"50"
            report "Repeated bytes: y3 failure"
            severity error;


        ----------------------------------------------------------------
        -- Alternating high-bit values
        ----------------------------------------------------------------

        a0 <= x"80";
        a1 <= x"00";
        a2 <= x"80";
        a3 <= x"00";

        wait for 1 ns;

        assert y0 = x"F0"
            report "80 00 80 00: y0 failure"
            severity error;

        assert y1 = x"A0"
            report "80 00 80 00: y1 failure"
            severity error;

        assert y2 = x"F0"
            report "80 00 80 00: y2 failure"
            severity error;

        assert y3 = x"A0"
            report "80 00 80 00: y3 failure"
            severity error;


        report "TLM P3 gate verification PASSED"
            severity note;

        wait;

    end process;

end architecture;
