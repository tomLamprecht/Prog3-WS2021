import { Component, EventEmitter, Input, OnInit, Output} from '@angular/core';

@Component({
  selector: 'item',
  templateUrl: './item.component.html',
  styleUrls: ['./item.component.scss'],
})
export class ItemComponent implements OnInit{
  @Input()
  title : string = "item";

  @Input()
  idItem : string;

  @Output()
  onTitleChange = new EventEmitter<string>();

  @Output()
  onDeleteItem = new EventEmitter();

  el : Element;

  onChange(event){
    this.onTitleChange.emit(event.target.value);
  }

  ngOnInit(){
  }

  ngAfterViewInit(){

    let element  = document.getElementById(this.idItem);

    if(element != null &&  element.className == "inputItem")
      element.focus();
    else{
      console.log("Couldnt find Item of id" + this.idItem);
    }

  }

  onDelete(){
    this.onDeleteItem.emit();
  }

  showDeleteButton : boolean = false;
}
