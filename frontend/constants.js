// Frontend constants and color mappings
const CONSTANTS = (() => {
    // Math helpers
    const hsvToRgb = (h, s, v) => {
        const sN = s/100, vN = v/255, c = vN*sN, x = c*(1-Math.abs(((h/60)%2)-1)), m = vN-c;
        let r=0, g=0, b=0;
        if(h<60){r=c;g=x} else if(h<120){r=x;g=c} else if(h<180){g=c;b=x}
        else if(h<240){g=x;b=c} else if(h<300){r=x;b=c} else {r=c;b=x}
        return [Math.round((r+m)*255), Math.round((g+m)*255), Math.round((b+m)*255)];
    };

    const rgbToHsv = (r, g, b) => {
        const max = Math.max(r,g,b), min = Math.min(r,g,b), d = max-min;
        let h = 0, s = max===0 ? 0 : Math.floor((d*100)/max);
        if(d!==0) {
            if(max===r) h = ((60*((g-b)*100/d)+36000)%36000)/100;
            else if(max===g) h = (60*((b-r)*100/d)+12000)/100;
            else h = (60*((r-g)*100/d)+24000)/100;
        }
        return { h:Math.floor(h), s, v:max };
    };

    const decodeInstrFromPixel = (p, hsv) => {
        if(p.a===0) return 'ERROR';
        if(p.r===0 && p.g===0 && p.b===0) return 'HALT';
        if(hsv.s<=20) return `DATA(${Math.max(p.r,p.g,p.b)})`;
        const hv = hsv.v>=128, h = hsv.h;
        if(hsv.s>60){ // EX Bank
            if(h<24) return hv?'TRAP_01':'TRAP_00'; if(h<48) return hv?'TRAP_03':'TRAP_02';
            if(h<72) return hv?'TRAP_05':'TRAP_04'; if(h<96) return hv?'TRAP_07':'TRAP_06';
            if(h<120) return hv?'OVER':'DUP';       if(h<144) return hv?'OR':'AND';
            if(h<168) return hv?'XOR':'NOT';        if(h<192) return hv?'ROTR':'ROT';
            if(h<216) return hv?'WRITE':'READ';     if(h<240) return hv?'LOAD_C':'STORE_C';
            if(h<264) return hv?'LOAD_D':'STORE_D';  if(h<288) return hv?'JLT':'JGT';
            if(h<312) return hv?'RET':'CALL';       if(h<336) return hv?'TRAP_13':'TRAP_12';
            return hv?'CLEAR':'DEPTH';
        }
        // Base Bank
        if(h<24) return hv?'RIGHT_SKIP':'RIGHT'; if(h<48) return hv?'DOWN_SKIP':'DOWN';
        if(h<72) return hv?'LEFT_SKIP':'LEFT';   if(h<96) return hv?'UP_SKIP':'UP';
        if(h<120) return hv?'SWAP':'POP';        if(h<144) return hv?'SUB':'ADD';
        if(h<168) return hv?'DIV':'MUL';        if(h<192) return hv?'NEG':'MOD';
        if(h<216) return hv?'DEC':'INC';        if(h<240) return hv?'LOAD_A':'STORE_A';
        if(h<264) return hv?'LOAD_B':'STORE_B';  if(h<288) return hv?'JZ':'JNZ';
        if(h<312) return hv?'IN_CHR':'IN_NUM';  if(h<336) return hv?'OUT_CHR':'OUT_NUM';
        return hv?'DEBUG':'NOP';
    };

    const OPS = {
        RIGHT:{desc:"Set dir RIGHT", stack:"—"}, RIGHT_SKIP:{desc:"Set dir RIGHT, skip", stack:"—"},
        DOWN:{desc:"Set dir DOWN", stack:"—"},   DOWN_SKIP:{desc:"Set dir DOWN, skip", stack:"—"},
        LEFT:{desc:"Set dir LEFT", stack:"—"},   LEFT_SKIP:{desc:"Set dir LEFT, skip", stack:"—"},
        UP:{desc:"Set dir UP", stack:"—"},       UP_SKIP:{desc:"Set dir UP, skip", stack:"—"},
        POP:{desc:"Pop top", stack:"[a]→[]"},    SWAP:{desc:"Swap top two", stack:"[a,b]→[b,a]"},
        ADD:{desc:"Add", stack:"[a,b]→[a+b]"},   SUB:{desc:"Sub", stack:"[a,b]→[a-b]"},
        MUL:{desc:"Mul", stack:"[a,b]→[a*b]"},   DIV:{desc:"Div", stack:"[a,b]→[a/b]"},
        MOD:{desc:"Mod", stack:"[a,b]→[a%b]"},   NEG:{desc:"Neg", stack:"[a]→[-a]"},
        INC:{desc:"Inc", stack:"[a]→[a+1]"},     DEC:{desc:"Dec", stack:"[a]→[a-1]"},
        STORE_A:{desc:"Pop to A", stack:"[a]→[]"},LOAD_A:{desc:"Push A", stack:"[]→[A]"},
        STORE_B:{desc:"Pop to B", stack:"[a]→[]"},LOAD_B:{desc:"Push B", stack:"[]→[B]"},
        STORE_C:{desc:"Pop to C", stack:"[a]→[]"},LOAD_C:{desc:"Push C", stack:"[]→[C]"},
        STORE_D:{desc:"Pop to D", stack:"[a]→[]"},LOAD_D:{desc:"Push D", stack:"[]→[D]"},
        JNZ:{desc:"Jump if top!=0", stack:"[a]→[]"},JZ:{desc:"Jump if top==0", stack:"[a]→[]"},
        JGT:{desc:"Jump if a>b", stack:"[a,b]→[]"}, JLT:{desc:"Jump if a<b", stack:"[a,b]→[]"},
        IN_NUM:{desc:"Read int", stack:"[]→[v]"}, IN_CHR:{desc:"Read char", stack:"[]→[v]"},
        OUT_NUM:{desc:"Print int", stack:"[a]→[]"},OUT_CHR:{desc:"Print char", stack:"[a]→[]"},
        NOP:{desc:"No op", stack:"—"},            DEBUG:{desc:"Debug state", stack:"—"},
        DUP:{desc:"Dup top", stack:"[a]→[a,a]"},  OVER:{desc:"Copy 2nd over top", stack:"[a,b]→[a,b,a]"},
        AND:{desc:"Bit AND", stack:"[a,b]→[a&b]"},OR:{desc:"Bit OR", stack:"[a,b]→[a|b]"},
        NOT:{desc:"Bit NOT", stack:"[a]→[~a]"},   XOR:{desc:"Bit XOR", stack:"[a,b]→[a^b]"},
        ROT:{desc:"Rot stack", stack:"[a,b,c]→[b,c,a]"},ROTR:{desc:"Rot right", stack:"[a,b,c]→[c,a,b]"},
        READ:{desc:"Read px", stack:"[x,y]→[v]"}, WRITE:{desc:"Write px", stack:"[v,x,y]→[]"},
        CALL:{desc:"Call sub", stack:"[…]→[…]"},  RET:{desc:"Ret from sub", stack:"[…]→[…]"},
        DEPTH:{desc:"Push depth", stack:"[]→[sp]"},CLEAR:{desc:"Clear stack", stack:"[…]→[]"},
        DATA:{desc:"Push V", stack:"[]→[V]"},     HALT:{desc:"Clean halt", stack:"—"},
        ERROR_HALT:{desc:"Error halt", stack:"—"}
    };

    const HUE_NAMES = ['Red','Orange','Yellow','Lime','Green','Teal','Cyan','Sky','Blue','Indigo','Violet','Purple','Magenta','Pink','Rose'];
    const BASE_INS = [['RIGHT','RIGHT_SKIP'],['DOWN','DOWN_SKIP'],['LEFT','LEFT_SKIP'],['UP','UP_SKIP'],['POP','SWAP'],['ADD','SUB'],['MUL','DIV'],['MOD','NEG'],['INC','DEC'],['STORE_A','LOAD_A'],['STORE_B','LOAD_B'],['JNZ','JZ'],['IN_NUM','IN_CHR'],['OUT_NUM','OUT_CHR'],['NOP','DEBUG']];
    const EX_INS = [['TRAP_00','TRAP_01'],['TRAP_02','TRAP_03'],['TRAP_04','TRAP_05'],['TRAP_06','TRAP_07'],['DUP','OVER'],['AND','OR'],['NOT','XOR'],['ROT','ROTR'],['READ','WRITE'],['STORE_C','LOAD_C'],['STORE_D','LOAD_D'],['JGT','JLT'],['CALL','RET'],['TRAP_12','TRAP_13'],['DEPTH','CLEAR']];

    const colors = [
        {name:'HALT (Black)',r:0,g:0,b:0,a:255,instruction:'HALT',bank:'Special',hsv:{h:0,s:0,v:0},pixelType:'HALT'},
        {name:'ERROR (Transparent)',r:0,g:0,b:0,a:0,instruction:'ERROR_HALT',bank:'Special',hsv:{h:0,s:0,v:0},pixelType:'ERROR'}
    ];

    [10,30,50,65,80,100,120,150,180,200,220,255].forEach(v=>{
        const rgb = hsvToRgb(0,10,v);
        colors.push({name:`DATA (push ${v})`,r:rgb[0],g:rgb[1],b:rgb[2],a:255,instruction:'DATA',bank:'Data',hsv:{h:0,s:10,v},pixelType:'DATA',dataValue:v});
    });

    BASE_INS.forEach((ins,i) => {
        const h = i*24;
        const rgbL = hsvToRgb(h,40,100), rgbH = hsvToRgb(h,40,200);
        colors.push({name:`${ins[0]} (${HUE_NAMES[i]})`,r:rgbL[0],g:rgbL[1],b:rgbL[2],a:255,instruction:ins[0],bank:'Base',hsv:{h,s:40,v:100},pixelType:'CODE'});
        colors.push({name:`${ins[1]} (${HUE_NAMES[i]})`,r:rgbH[0],g:rgbH[1],b:rgbH[2],a:255,instruction:ins[1],bank:'Base',hsv:{h,s:40,v:200},pixelType:'CODE'});
    });

    EX_INS.forEach((ins,i) => {
        const h = i*24;
        const rgbL = hsvToRgb(h,80,100), rgbH = hsvToRgb(h,80,200);
        colors.push({name:`${ins[0]} (${HUE_NAMES[i]} EX)`,r:rgbL[0],g:rgbL[1],b:rgbL[2],a:255,instruction:ins[0],bank:'Extended',hsv:{h,s:80,v:100},pixelType:'CODE'});
        colors.push({name:`${ins[1]} (${HUE_NAMES[i]} EX)`,r:rgbH[0],g:rgbH[1],b:rgbH[2],a:255,instruction:ins[1],bank:'Extended',hsv:{h,s:80,v:200},pixelType:'CODE'});
    });

    return { colors, OPS, hsvToRgb, rgbToHsv, decodeInstrFromPixel };
})();
