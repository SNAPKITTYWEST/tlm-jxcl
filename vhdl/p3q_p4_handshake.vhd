library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity p3q_p4_handshake is
    generic (
        MAX_SHOTS       : natural := 1024;
        MAX_LATENCY_CYC : natural := 1000000
    );
    port (
        clk           : in  std_logic;
        rst_n         : in  std_logic;

        p4_event_valid : in  std_logic;
        p4_event_id    : in  std_logic_vector(31 downto 0);
        p4_event_type  : in  std_logic_vector(7 downto 0);
        p4_event_params: in  std_logic_vector(255 downto 0);
        p4_deadline    : in  std_logic_vector(63 downto 0);
        p4_event_ready : out std_logic;

        p4_result_valid: out std_logic;
        p4_result_id   : out std_logic_vector(31 downto 0);
        p4_result_status: out std_logic_vector(7 downto 0);
        p4_result_data : out std_logic_vector(511 downto 0);
        p4_result_shots: out std_logic_vector(31 downto 0);
        p4_result_ready: in  std_logic;

        qsim_cmd_valid : out std_logic;
        qsim_cmd_type  : out std_logic_vector(7 downto 0);
        qsim_cmd_qubits: out std_logic_vector(15 downto 0);
        qsim_cmd_shots : out std_logic_vector(31 downto 0);
        qsim_cmd_params: out std_logic_vector(511 downto 0);
        qsim_cmd_ready : in  std_logic;

        qsim_rsp_valid : in  std_logic;
        qsim_rsp_id    : in  std_logic_vector(31 downto 0);
        qsim_rsp_status: in  std_logic_vector(7 downto 0);
        qsim_rsp_data  : in  std_logic_vector(511 downto 0);
        qsim_rsp_shots : in  std_logic_vector(31 downto 0);
        qsim_rsp_ready : out std_logic
    );
end entity p3q_p4_handshake;

architecture rtl of p3q_p4_handshake is
    type state_t is (IDLE, TRANSLATE, ISSUE_CMD, WAIT_RSP, FORMAT_RSP, DONE);
    signal state : state_t := IDLE;

    signal event_id_reg    : std_logic_vector(31 downto 0);
    signal event_type_reg  : std_logic_vector(7 downto 0);
    signal deadline_reg    : std_logic_vector(63 downto 0);
    signal cycle_counter   : unsigned(63 downto 0);

begin
    process(clk, rst_n)
    begin
        if rst_n = '0' then
            cycle_counter <= (others => '0');
        elsif rising_edge(clk) then
            if state = ISSUE_CMD or state = WAIT_RSP then
                cycle_counter <= cycle_counter + 1;
            else
                cycle_counter <= (others => '0');
            end if;
        end if;
    end process;

    process(clk, rst_n)
    begin
        if rst_n = '0' then
            state           <= IDLE;
            p4_event_ready  <= '1';
            p4_result_valid <= '0';
            qsim_cmd_valid  <= '0';
            qsim_rsp_ready  <= '0';
        elsif rising_edge(clk) then
            case state is
                when IDLE =>
                    p4_event_ready  <= '1';
                    p4_result_valid <= '0';
                    if p4_event_valid = '1' then
                        event_id_reg   <= p4_event_id;
                        event_type_reg <= p4_event_type;
                        deadline_reg   <= p4_deadline;
                        p4_event_ready <= '0';
                        state          <= TRANSLATE;
                    end if;

                when TRANSLATE =>
                    case event_type_reg is
                        when x"01" =>
                            qsim_cmd_type   <= x"01";
                            qsim_cmd_qubits <= x"0100";
                            qsim_cmd_shots  <= std_logic_vector(to_unsigned(1, 32));
                        when x"02" =>
                            qsim_cmd_type   <= x"02";
                            qsim_cmd_qubits <= x"0080";
                            qsim_cmd_shots  <= std_logic_vector(to_unsigned(1, 32));
                        when x"03" =>
                            qsim_cmd_type   <= x"03";
                            qsim_cmd_qubits <= x"0200";
                            qsim_cmd_shots  <= std_logic_vector(to_unsigned(MAX_SHOTS, 32));
                        when x"04" =>
                            qsim_cmd_type   <= x"04";
                            qsim_cmd_qubits <= x"0101";
                            qsim_cmd_shots  <= std_logic_vector(to_unsigned(MAX_SHOTS, 32));
                        when others =>
                            qsim_cmd_type   <= x"FF";
                            qsim_cmd_qubits <= x"0000";
                            qsim_cmd_shots  <= x"00000000";
                    end case;
                    qsim_cmd_params <= p4_event_params;
                    state <= ISSUE_CMD;

                when ISSUE_CMD =>
                    qsim_cmd_valid <= '1';
                    if qsim_cmd_ready = '1' then
                        qsim_cmd_valid <= '0';
                        qsim_rsp_ready <= '1';
                        state <= WAIT_RSP;
                    end if;

                when WAIT_RSP =>
                    if qsim_rsp_valid = '1' then
                        qsim_rsp_ready <= '0';
                        state <= FORMAT_RSP;
                    elsif cycle_counter >= unsigned(deadline_reg) then
                        qsim_rsp_ready    <= '0';
                        p4_result_status  <= x"01";
                        p4_result_id      <= event_id_reg;
                        p4_result_data    <= (others => '0');
                        p4_result_shots   <= (others => '0');
                        state <= DONE;
                    end if;

                when FORMAT_RSP =>
                    p4_result_id     <= qsim_rsp_id;
                    p4_result_status <= qsim_rsp_status;
                    p4_result_data   <= qsim_rsp_data;
                    p4_result_shots  <= qsim_rsp_shots;
                    state <= DONE;

                when DONE =>
                    p4_result_valid <= '1';
                    if p4_result_ready = '1' then
                        p4_result_valid <= '0';
                        state <= IDLE;
                    end if;
            end case;
        end if;
    end process;
end architecture rtl;
