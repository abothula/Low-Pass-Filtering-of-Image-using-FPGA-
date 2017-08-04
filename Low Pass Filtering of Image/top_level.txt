-----------------------------------------------------------
-- Copyright (C) 2009-2012 Chris McClelland
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Lesser General Public License for more details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------------
-- Additional changes/comments by Cristinel Ababei, August 23 2012:	 
--
-- From the host, writes to R0 are simply displayed on the Atlys board's 
-- eight LEDs. Reads from R0 return the state of the board's eight slide 
-- switches. Writes to R1 and R2 are registered and may be read back. 
-- The circuit implemented on the FPGA simply multiplies the R1 with R2 
-- and places the result in R3. Only reads, from host side, are allowed 
-- from from R3; that is an attempt to write into R3 will have no effect.
-- When you input, from host side, data into R1 and R2, data should
-- represent numbers that can be represented on 4 bits only. Because
-- data will have to be input (will be done via the flcli application)
-- in hex, writing for example 07 or A7 into R1 will have the same effect 
-- as writing 07 because the four MSB will be discarded inside the
-- VHDL application on FPGA.
------------------------------------------------------------------------------
-library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity top_level is
	port(
		-- FX2 interface -----------------------------------------------------------------------------
		fx2Clk_in     : in    std_logic;                    -- 48MHz clock from FX2
		fx2Addr_out   : out   std_logic_vector(1 downto 0); -- select FIFO: "10" for EP6OUT, "11" for EP8IN
		fx2Data_io    : inout std_logic_vector(7 downto 0); -- 8-bit data to/from FX2

		-- When EP6OUT selected:
		fx2Read_out   : out   std_logic;                    -- asserted (active-low) when reading from FX2
		fx2OE_out     : out   std_logic;                    -- asserted (active-low) to tell FX2 to drive bus
		fx2GotData_in : in    std_logic;                    -- asserted (active-high) when FX2 has data for us

		-- When EP8IN selected:
		fx2Write_out  : out   std_logic;                    -- asserted (active-low) when writing to FX2
		fx2GotRoom_in : in    std_logic;                    -- asserted (active-high) when FX2 has room for more data from us
		fx2PktEnd_out : out   std_logic;                    -- asserted (active-low) when a host read needs to be committed early

		-- Onboard peripherals -----------------------------------------------------------------------
		led_out       : out   std_logic_vector(7 downto 0); -- eight LEDs
		slide_sw_in   : in    std_logic_vector(7 downto 0)  -- eight slide switches
	);
end top_level;

architecture behavioural of top_level is
	-- Channel read/write interface -----------------------------------------------------------------
	signal chanAddr  : std_logic_vector(6 downto 0);  -- the selected channel (0-127)

	-- Host >> FPGA pipe:
	signal h2fData   : std_logic_vector(7 downto 0);  -- data lines used when the host writes to a channel
	signal h2fValid  : std_logic;                     -- '1' means "on the next clock rising edge, please accept the data on h2fData"
	signal h2fReady  : std_logic;                     -- channel logic can drive this low to say "I'm not ready for more data yet"

	-- Host << FPGA pipe:
	signal f2hData   : std_logic_vector(7 downto 0);  -- data lines used when the host reads from a channel
	signal f2hValid  : std_logic;                     -- channel logic can drive this low to say "I don't have data ready for you"
	signal f2hReady  : std_logic;                     -- '1' means "on the next clock rising edge, put your next byte of data on f2hData"
	-- ----------------------------------------------------------------------------------------------

	-- Needed so that the comm_fpga_fx2 module can drive both fx2Read_out and fx2OE_out
	signal fx2Read                 : std_logic;

	-- Registers implementing the channels
	 type array_type_1 is array (0 to 9) of std_logic_vector(11 downto 0);
	 signal reg : array_type_1 := (others=> (others=>'0'));
	 signal reg_next : array_type_1 := (others=> (others=>'0'));
	 signal div : std_logic_vector(11 downto 0)  := x"009";
	
begin													-- BEGIN_SNIPPET(registers)
	-- Infer registers
	process(fx2Clk_in)
	begin
		if ( rising_edge(fx2Clk_in) ) then
			--checksum <= checksum_next;
			FOR i IN 0 to 9 LOOP
          		reg(i) <= reg_next(i);
        	END LOOP;
		end if;
	end process;

	-- Drive register inputs for each channel when the host is writing
	reg_next(0)(7 downto 0) <= h2fData when chanAddr = "0000000" and h2fValid = '1' else reg(0)(7 downto 0);
	reg_next(1)(7 downto 0) <= h2fData when chanAddr = "0000001" and h2fValid = '1' else reg(1)(7 downto 0);
	reg_next(2)(7 downto 0) <= h2fData when chanAddr = "0000010" and h2fValid = '1' else reg(2)(7 downto 0);
	reg_next(3)(7 downto 0) <= h2fData when chanAddr = "0000011" and h2fValid = '1' else reg(3)(7 downto 0);
	reg_next(4)(7 downto 0) <= h2fData when chanAddr = "0000100" and h2fValid = '1' else reg(4)(7 downto 0);
	reg_next(5)(7 downto 0) <= h2fData when chanAddr = "0000101" and h2fValid = '1' else reg(5)(7 downto 0);
	reg_next(6)(7 downto 0) <= h2fData when chanAddr = "0000110" and h2fValid = '1' else reg(6)(7 downto 0);
	reg_next(7)(7 downto 0) <= h2fData when chanAddr = "0000111" and h2fValid = '1' else reg(7)(7 downto 0);
	reg_next(8)(7 downto 0) <= h2fData when chanAddr = "0001000" and h2fValid = '1' else reg(8)(7 downto 0);
	reg_next(9) <= std_logic_vector((unsigned(reg(0))+unsigned(reg(1))+unsigned(reg(2))+unsigned(reg(3))+unsigned(reg(4))+unsigned(reg(5))+unsigned(reg(6))+unsigned(reg(7))+unsigned(reg(8)))/unsigned(div));
		
	with chanAddr select f2hData <=
		reg(0)(7 downto 0)      	when "0000000",
		reg(1)(7 downto 0)      	when "0000001",
		reg(2)(7 downto 0)      	when "0000010",
		reg(3)(7 downto 0)      	when "0000011",
		reg(4)(7 downto 0)      	when "0000100",
		reg(5)(7 downto 0)      	when "0000101",
		reg(6)(7 downto 0)      	when "0000110",
		reg(7)(7 downto 0)      	when "0000111",
		reg(8)(7 downto 0)      	when "0001000",
		reg(9)(7 downto 0)      	when "0001001",
		x"00"		 						when others;

	-- Assert that there's always data for reading, and always room for writing
	f2hValid <= '1';
	h2fReady <= '1';								--END_SNIPPET(registers)

	-- CommFPGA module
	fx2Read_out <= fx2Read;
	fx2OE_out <= fx2Read;
	fx2Addr_out(1) <= '1';  -- Use EP6OUT/EP8IN, not EP2OUT/EP4IN.
	comm_fpga_fx2 : entity work.comm_fpga_fx2
		port map(
			-- FX2 interface
			fx2Clk_in      => fx2Clk_in,
			fx2FifoSel_out => fx2Addr_out(0),
			fx2Data_io     => fx2Data_io,
			fx2Read_out    => fx2Read,
			fx2GotData_in  => fx2GotData_in,
			fx2Write_out   => fx2Write_out,
			fx2GotRoom_in  => fx2GotRoom_in,
			fx2PktEnd_out  => fx2PktEnd_out,

			-- Channel read/write interface
			chanAddr_out   => chanAddr,
			h2fData_out    => h2fData,
			h2fValid_out   => h2fValid,
			h2fReady_in    => h2fReady,
			f2hData_in     => f2hData,
			f2hValid_in    => f2hValid,
			f2hReady_out   => f2hReady
		);

	-- LEDs
	led_out <= reg(0)(7 downto 0);
end behavioural;