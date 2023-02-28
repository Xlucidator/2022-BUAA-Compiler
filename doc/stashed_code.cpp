case IROp::ADD:{
    string rd = useNameFromReg(peek.res, USE_TO);
    string rs = useNameFromReg(peek.label1, USE_FROM);
    string rt = useName(peek.label2, USE_FROM);
    inst = "add " + rd + " " + rs + " " + rt;
    textSeg.emplace_back(inst);
    nextIR();
    break;
}
case IROp::MIN: {
    string rd = useNameFromReg(peek.res, USE_TO);
    string rs = useNameFromReg(peek.label1, USE_FROM);
    string rt = useName(peek.label2, USE_FROM);
    inst = "sub " + rd + " " + rs + " " + rt;
    textSeg.emplace_back(inst);
    nextIR();
    break;
}
case IROp::MUL: {
    string rd = useNameFromReg(peek.res, USE_TO);
    string rs = useNameFromReg(peek.label1, USE_FROM);
    string rt = useName(peek.label2, USE_FROM);
    inst = "mul " + rd + " " + rs + " " + rt;   // rd will get low-32bit of the res
    textSeg.emplace_back(inst);
    nextIR();
    break;
}
case IROp::DIV: {
    string rd = useNameFromReg(peek.res, USE_TO);
    string rs = useNameFromReg(peek.label1, USE_FROM);
    string rt = useName(peek.label2, USE_FROM);
    inst = "div " + rd + " " + rs + " " + rt;   // rd will get low-32bit of the res
    textSeg.emplace_back(inst);
    nextIR();
    break;
}
case IROp::AND: {
    string rd = useNameFromReg(peek.res, USE_TO);
    string rs = useNameFromReg(peek.label1, USE_FROM);
    string rt = useName(peek.label2, USE_FROM);
    inst = "and " + rd + " " + rs + " " + rt;
    textSeg.emplace_back(inst);
    nextIR();
    break;
}
case IROp::OR: {
    string rd = useNameFromReg(peek.res, USE_TO);
    string rs = useNameFromReg(peek.label1, USE_FROM);
    string rt = useName(peek.label2, USE_FROM);
    inst = "or " + rd + " " + rs + " " + rt;
    textSeg.emplace_back(inst);
    nextIR();
    break;
}
