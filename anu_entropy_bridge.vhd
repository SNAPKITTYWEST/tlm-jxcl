library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity anu_entropy_bridge is
    port (
        clk : in std_logic;
        rst_n : in std_logic;
        
        -- Raw Vacuum Fluctuation Input (Analog-to-Digital converted)
        vac_raw : in std_logic_vector(15 downto 0);
        vac_valid : in std_logic;
        
        -- T=SQL Temporal Indexing
        timestamp_in : in std_logic_vector(63 downto 0);
        
        -- Output to P3Q Quantum Interface
        q_seed_valid : out std_logic;
        q_seed_data : out std_logic_vector(255 downto 0);
        t_sql_index : out std_logic_vector(31 downto 0)
    );
end entity anu_entropy_bridge;

architecture rtl of anu_entropy_bridge is
    signal seed_reg : std_logic_vector(255 downto 0) := (others => '0');
    signal bit_count : unsigned(7 downto 0) := (others => '0');
    
    -- T=SQL Hash Constant
    constant T_SQL_SALT : std_logic_vector(31 downto 0) := x"DEADBEEF";
begin

    -- ANu Normalization Process: Convert Raw Fluctuations to Bitstream
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            seed_reg <= (others => '0');
            bit_count <= (others => '0');
            q_seed_valid <= '0';
        elsif rising_edge(clk) then
            q_seed_valid <= '0';
            
            if vac_valid = '1' then
                -- Normalize: MSB of raw fluctuation becomes the seed bit
                seed_reg(to_integer(unsigned(bit_count))) <= vac_raw(15);
                
                if bit_count = 255 then
                    bit_count <= (others => '0');
                    q_seed_valid <= '1'; -- Seed buffer full, trigger P3Q
                else
                    bit_count <= bit_count + 1;
                end if;
            end if;
        end if;
    end process;

    -- T=SQL Indexing: Simple XOR-sum hash of timestamp for Worm Chain lookup
    process(timestamp_in)
    begin
        -- Mapping T (Time) to SQL (Structured Index)
        -- t_sql_index = (timestamp_high ^ timestamp_low) ^ SALT
        t_sql_index <= (timestamp_in(63 downto 32) xor timestamp_in(31 downto 0)) xor T_SQL_SALT;
    end process;

end architecture rtl;
