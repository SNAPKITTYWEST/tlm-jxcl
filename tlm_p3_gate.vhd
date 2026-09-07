library ieee;
use ieee.std_logic_1164.all;

package tlm_p3_components is

    component xor_gate is
        port (
            a : in std_logic;
            b : in std_logic;
            y : out std_logic
        );
    end component;

    component and_gate is
        port (
            a : in std_logic;
            b : in std_logic;
            y : out std_logic
        );
    end component;

    component xtime_gate is
        port (
            b : in std_logic_vector(7 downto 0);
            y : out std_logic_vector(7 downto 0)
        );
    end component;

end package;


library ieee;
use ieee.std_logic_1164.all;

entity xor_gate is
    port (
        a : in std_logic;
        b : in std_logic;
        y : out std_logic
    );
end entity;

architecture structural of xor_gate is
begin
    y <= a xor b;
end architecture;


library ieee;
use ieee.std_logic_1164.all;

entity and_gate is
    port (
        a : in std_logic;
        b : in std_logic;
        y : out std_logic
    );
end entity;

architecture structural of and_gate is
begin
    y <= a and b;
end architecture;


library ieee;
use ieee.std_logic_1164.all;

entity xtime_gate is
    port (
        b : in std_logic_vector(7 downto 0);
        y : out std_logic_vector(7 downto 0)
    );
end entity;

architecture structural of xtime_gate is

    signal r4 : std_logic;
    signal r3 : std_logic;
    signal r1 : std_logic;
    signal r0 : std_logic;

begin

    -- 0x1B = 00011011
    -- Reduction bits: r4 = b7, r3 = b7, r1 = b7, r0 = b7

    r4 <= b(7) and '1';
    r3 <= b(7) and '1';
    r1 <= b(7) and '1';
    r0 <= b(7) and '1';

    y(7) <= b(6);
    y(6) <= b(5);
    y(5) <= b(4) xor r4;
    y(4) <= b(3) xor r3;
    y(3) <= b(2);
    y(2) <= b(1);
    y(1) <= b(0) xor r1;
    y(0) <= r0;

end architecture;


library ieee;
use ieee.std_logic_1164.all;

entity tlm_p3_gate is
    port (
        a0 : in std_logic_vector(7 downto 0);
        a1 : in std_logic_vector(7 downto 0);
        a2 : in std_logic_vector(7 downto 0);
        a3 : in std_logic_vector(7 downto 0);

        y0 : out std_logic_vector(7 downto 0);
        y1 : out std_logic_vector(7 downto 0);
        y2 : out std_logic_vector(7 downto 0);
        y3 : out std_logic_vector(7 downto 0)
    );
end entity;


architecture structural of tlm_p3_gate is

    signal xor_a0_a1 : std_logic_vector(7 downto 0);
    signal xor_a1_a2 : std_logic_vector(7 downto 0);
    signal xor_a2_a3 : std_logic_vector(7 downto 0);
    signal xor_a3_a0 : std_logic_vector(7 downto 0);

    signal t : std_logic_vector(7 downto 0);

    signal t01 : std_logic_vector(7 downto 0);
    signal t23 : std_logic_vector(7 downto 0);

    signal xtime_0 : std_logic_vector(7 downto 0);
    signal xtime_1 : std_logic_vector(7 downto 0);
    signal xtime_2 : std_logic_vector(7 downto 0);
    signal xtime_3 : std_logic_vector(7 downto 0);

    signal y0_tmp : std_logic_vector(7 downto 0);
    signal y1_tmp : std_logic_vector(7 downto 0);
    signal y2_tmp : std_logic_vector(7 downto 0);
    signal y3_tmp : std_logic_vector(7 downto 0);

begin

    xor_a0_a1 <= a0 xor a1;
    xor_a1_a2 <= a1 xor a2;
    xor_a2_a3 <= a2 xor a3;
    xor_a3_a0 <= a3 xor a0;

    t01 <= a0 xor a1;
    t23 <= a2 xor a3;
    t <= t01 xor t23;

    XT0 : entity work.xtime_gate(structural)
        port map (
            b => xor_a0_a1,
            y => xtime_0
        );

    XT1 : entity work.xtime_gate(structural)
        port map (
            b => xor_a1_a2,
            y => xtime_1
        );

    XT2 : entity work.xtime_gate(structural)
        port map (
            b => xor_a2_a3,
            y => xtime_2
        );

    XT3 : entity work.xtime_gate(structural)
        port map (
            b => xor_a3_a0,
            y => xtime_3
        );

    y0_tmp <= a0 xor t;
    y1_tmp <= a1 xor t;
    y2_tmp <= a2 xor t;
    y3_tmp <= a3 xor t;

    y0 <= y0_tmp xor xtime_0;
    y1 <= y1_tmp xor xtime_1;
    y2 <= y2_tmp xor xtime_2;
    y3 <= y3_tmp xor xtime_3;

end architecture;
