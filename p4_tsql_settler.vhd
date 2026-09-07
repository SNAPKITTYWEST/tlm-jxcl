library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity p4_tsql_settler is
    port (
        clk : in std_logic;
        rst_n : in std_logic;

        t_sql_index : in std_logic_vector(31 downto 0);
        q_seed_valid : in std_logic;

        settle_valid : out std_logic;
        settle_id : out std_logic_vector(63 downto 0);
        settle_index : out std_logic_vector(31 downto 0)
    );
end entity p4_tsql_settler;

architecture rtl of p4_tsql_settler is
    signal sequence_cnt : unsigned(63 downto 0) := (others => '0');
begin

    process(clk, rst_n)
    begin
        if rst_n = '0' then
            sequence_cnt <= (others => '0');
            settle_valid <= '0';
        elsif rising_edge(clk) then
            settle_valid <= '0';

            if q_seed_valid = '1' then
                sequence_cnt <= sequence_cnt + 1;
                settle_id <= std_logic_vector(sequence_cnt + 1);
                settle_index <= t_sql_index;
                settle_valid <= '1';
            end if;
        end if;
    end process;

end architecture rtl;
