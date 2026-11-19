#pragma once
void drawAgentOverlay(){
    auto view=personal.view;auto title=view==PersonalView::Tasks?L"Tarefas pendentes":view==PersonalView::WindowConsent?L"Autorizar janela":view==PersonalView::OperationReview?L"Detalhes da ação":L"Sua participação";
    text(title,204,87,454,21,0xF5F5F8,true);iconButton(3191,0xE711,view==PersonalView::Tasks?L"Fechar painel":L"Cancelar",676,87,30,30);
    if(view==PersonalView::Tasks){
        if(personal.rows.empty())alignedText(L"Nenhuma tarefa pendente",204,240,508,32,18,0xD8D8E1);
        for(int row=0;row<4;row++){int index=personal.listPage*4+row;if(index>=static_cast<int>(personal.rows.size()))break;auto& record=personal.rows[index];float y=144+row*58.f;button(3430+row,L"",204,y,456,50);auto goal=record.contains("task")?record["task"].value("goal",""):"";alignedText(hext::wide(goal),218,y+3,422,23,13,0xEEEEF4,true,false);alignedText(L"Retomar com novas verificações",218,y+27,422,18,11,0xB7B7C1,false,false);hits.back().name=L"Retomar tarefa: "+hext::wide(goal);iconButton(3480+row,0xE74D,L"Excluir tarefa",672,y+9,32,32);}
        personalParagraph(L"A retomada mantém o pedido original. Etapas com resultado desconhecido serão verificadas; autorizações antigas não são reutilizadas.",220,382,476,42,11);
        if(personal.rows.size()>4){button(3098,L"Anterior",204,432,108,28);button(3099,L"Próximos",324,432,108,28);alignedText(std::to_wstring((personal.rows.size()+3)/4)+L"/"+std::to_wstring(personal.listPage+1),448,432,80,28,10,0x9A9AA6,false,false);}
    }else if(view==PersonalView::WindowConsent){
        auto found=personal.windows.find(personal.pendingWindow);auto name=found==personal.windows.end()?L"Janela indisponível":hext::wide(found->second.value("title",""));
        symbol(0xE737,440,170,32,0xC9C9D2);personalParagraph(name,230,232,456,60,17,0xF0F0F5);
        personalParagraph(L"Permitir que o Hext leia os controles desta janela e envie esse conteúdo ao provedor de IA? Campos de senha são excluídos. Clicar ou preencher controles exige autorização própria.",236,300,444,88,13);
        button(3090,L"Cancelar",204,414,244,35);button(3450,L"Autorizar esta janela",460,414,252,35,true);
    }else if(view==PersonalView::Intervention){
        symbol(0xE72E,440,169,32,0xC9C9D2);auto instruction=personal.inFlight?hext::wide(personal.inFlight->args.value("instruction","")):L"Conclua a etapa no aplicativo oficial.";
        personalParagraph(instruction,236,235,444,90,16,0xEEEEF4);
        personalParagraph(L"Faça a autenticação diretamente no aplicativo. O Hext verifica o estado visível autorizado e continua quando encontrar a condição esperada. Sua senha não é solicitada ao assistente.",236,331,444,67,12);
        button(3423,L"Pausar e guardar",204,414,244,35);button(3090,L"Interromper",460,414,252,35);
    }else{
        auto detail=personalOperationDetails();int pages=std::max(1,static_cast<int>((detail.size()+649)/650));personal.reviewPage=std::clamp(personal.reviewPage,0,pages-1);personalParagraph(detail.substr(personal.reviewPage*650,650),220,145,476,251,12,0xDDDEE6);
        button(3441,L"Anterior",204,414,120,32);button(3442,L"Próximos",336,414,120,32);alignedText(std::to_wstring(pages)+L"/"+std::to_wstring(personal.reviewPage+1),467,414,70,32,10,0x9A9AA6);button(3443,L"Voltar",552,414,160,32);
    }
    if(!personal.notice.empty())text(personal.notice,212,450,492,10,0xD9D9E2);
}
