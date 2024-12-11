`define HS_F_DIV		1050
`define VS_F_DIV		838500
`define HS_DUR			156		//total length of each horizontal signal
`define HS_P_DUR		21			//hs pulse duration, 100s of ns
`define HS_B_DUR		23			//back porch duration, 100s of ns
`define HS_D_DUR		119		//display duration, 100s of ns
`define HS_F_DUR		4			//front porch duration, 100s of ns
`define VS_DUR			1066		//total length of the frame in lines
`define VS_P_DUR		3			//vs pulse duration, lines
`define VS_B_DUR		38			//back porch duration, lines
`define VS_D_DUR		1024		//display duration, lines
`define VS_F_DUR		1			//front porch duration, lines
`define N				15


module VGA_Checker(
		input		[3:0]		KEY,
		input					CLOCK_50,
		input		[35:0]	GPIO,
		
		output	[7:0]		VGA_R,
		output	[7:0]		VGA_G,
		output	[7:0]		VGA_B,
		output				VGA_CLK,
		output				VGA_BLANK_N,
		output				VGA_SYNC_N,
		
		output				VGA_HS,
		output				VGA_VS
);
		logic		[$clog2(`HS_F_DIV)-1:0]	HS_counter, next_HS_counter;
		logic		[$clog2(`VS_F_DIV)-1:0]	VS_counter, next_VS_counter;
		
		logic		[7:0]		red,		next_red,	cr;
		logic		[7:0]		green,	next_green,	cg;
		logic		[7:0]		blue,		next_blue,	cb;
		logic					hs,		next_hs;
		logic					vs,		next_vs;
		logic					blank,	next_blank;
		logic					sync,		next_sync;
		
		logic					spi_clk, csel, mosi, miso;
		
		logic	[`N-1:0][7:0]	regs, next_regs;
		logic [15:0]		 	packet, next_packet;
		logic [4:0]				spi_cnt, next_spi_cnt;
		
		logic	[`N-1:0][$clog2(`VS_F_DIV)-1:0]	cmp_regs;
		
		assign spi_clk			= GPIO[0];
		assign csel				= GPIO[2];
		assign mosi				= GPIO[4];
		assign miso				= GPIO[6];
		
		assign VGA_R			= red;
		assign VGA_G			= green;
		assign VGA_B			= blue;
		assign VGA_BLANK_N	= blank;
		assign VGA_SYNC_N		= sync;
		assign VGA_HS			= hs;
		assign VGA_VS			= vs;
		assign VGA_CLK			= CLOCK_50;
		//assign GPIO[1]			= VS_counter[$clog2(`VS_F_DIV)-1];
		generate
			genvar j;
			for(j = 0; j < `N; j++) begin : compCreation
				//assign cmp_regs[j][$clog2(`VS_F_DIV)-1]							= '1;
				assign cmp_regs[j][$clog2(`VS_F_DIV)-1:$clog2(`VS_F_DIV)-8] = regs[j];
				assign cmp_regs[j][$clog2(`VS_F_DIV)-9:0]						= '1;
				//assign cmp_regs[j] = {regs[j],'0};
			end
		endgenerate
		
		always_comb begin
			cr						= '1;
			cg						= '1;
			cb						= '1;
			
			for(int l = 0; l < 64; l++) begin
				if(VS_counter[19:14] == l) begin
					cr = {VS_counter[15], 7'b1100000};
					cg = {VS_counter[15], 7'b1100000};
					cb = {VS_counter[15], 7'b1100000};
					for(int n = 0;  n < 25; n++) begin
						if(VS_counter[19:14] == n) begin
							cr = 255;
							cg = 0 + (5 * n);
							cb = 0;
						end
					end
					for(int m = 25; m < 51; m++) begin
						if(VS_counter[19:14] == m) begin
							cr = m >= 42 ? '0						: 255 - ((m-25)*15);
							cg = m >= 42 ? 255 - ((m-42)*6)	: 135 + ((m-25)*7);
							cb = m >= 42 ? 0 + ((m-42)*31)	: '0;
						end
					end
				end
			end
			
			
			
			
			
			
		
			next_HS_counter	= HS_counter;
			next_red				= red;
			next_green			= green;
			next_blue			= blue;
			if(HS_counter >= `HS_F_DIV) begin //clock division for VS
				next_HS_counter = '0;
			end else begin
				next_HS_counter = HS_counter + 1;
			end
			
			next_VS_counter = VS_counter;
			if(VS_counter >= `VS_F_DIV) begin //clock division for VS
				next_VS_counter = '0;
			end else begin
				next_VS_counter = VS_counter + 1;
			end
			
			for(int i = 0; i < `N; i++) begin //vertical bars
				if((HS_counter-230) <= (((i+1) * 53)) && (HS_counter-230) >= (((i) * (53)))) begin
					//if((VS_counter-200000) <= cmp_regs[i]) begin top down
					if(835120-VS_counter >= cmp_regs[i]) begin //bottom up
						next_red		= '0;//8'b10000000;
						next_green	= '0;//8'b10000000;
						next_blue	= '0;//8'b10000000;
					end else begin
						next_red		= cr;
						next_green	= cg;
						next_blue	= cb;
					end
				end 
			end
			
			for(int j = 0; j < `N; j++) begin
				if ((HS_counter-230) == (((j+1) * 53)) || (HS_counter-230) == (((j+1) * 53)+1) || (HS_counter-230) == (((j+1) * 53)-1)) begin
					next_red		= '0;
					next_green	= '0;
					next_blue	= '0;
				end
			end
			

			if(VS_counter[14] && &VS_counter[13:11]) begin
				next_red		= '0;
				next_green	= '0;
				next_blue	= '0;
			end
			
			
			if(HS_counter <= ((`HS_F_DIV *`HS_P_DUR)/`HS_DUR)) begin //creates the HS pulse
				next_hs = 0;
			end else begin
				next_hs = 1;
			end
			
			if(HS_counter <= 230 || HS_counter >= 1020) begin
				next_blank = 0;
			end else begin
				next_blank = 1;
			end
			
			
			if(VS_counter <= 6240) begin//((`VS_F_DIV * `VS_P_DUR)/`VS_DUR)) begin //creates the VS pulse
				next_vs = 0;
			end else begin
				next_vs = 1;
			end
			
			if(VS_counter <= 36400 || VS_counter >= 835120) begin
				next_sync = 0;
			end else begin
				next_sync = 1;
			end
		end
		
		always_ff @(posedge CLOCK_50) begin
			if(~KEY[0]) begin
				HS_counter <= '0;
				VS_counter <= '0;
			
				red	<=	'0;
				green <=	'0;
				blue	<=	'0;
				
				blank	<=	'0;
				sync	<=	'0;
				hs		<=	'0;
				vs		<= '0;
				regs	<=	'0;
			end else begin
				HS_counter <= next_HS_counter;
				VS_counter <= next_VS_counter;
				
				red	<=	next_red;
				green <=	next_green;
				blue	<=	next_blue;
				
				blank	<=	next_blank;
				sync	<=	next_sync;
				hs		<=	next_hs;
				vs		<= next_vs;
				regs	<=	next_regs;
			end
		end
		
		

		always_comb begin
			next_packet 	= packet;
			next_regs		= regs;
			
			if(csel) begin
				next_packet = {mosi, packet[15:1]};
			end
			
			if(~csel) begin
				next_regs[packet[7:0]] = packet[15:8];
			end
		end
		
		always_ff @(posedge spi_clk) begin
			 packet	<= next_packet;
		end
endmodule